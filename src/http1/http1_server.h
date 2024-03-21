/*
 * Copyright (c) 2023 Xiaoming Zhang
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <exception>
#include <type_traits>
#include <utility>
#include <iostream>

#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <exec/linux/io_uring_context.hpp>
#include <exec/timed_scheduler.hpp>
#include <exec/repeat_effect_until.hpp>

#include "utils/timeout.h"
#include "utils/if_then_else.h"
#include "utils/flat_buffer.h"
#include "http1/http1_common.h"
#include "http1/http1_error.h"
#include "http1/http1_message_parser.h"
#include "http1/http1_response.h"
#include "http1/http1_concept.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

namespace net::http1 {
  struct send_state {
    std::string start_line_and_headers;
    http1::response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
  };

  namespace _recv_request {
    template <http1_request Request>
    struct recv_state {
      Request request{};
      message_parser<Request> parser{&request};
      util::flat_buffer<8192> buffer{};
      Request::duration remaining_time{0};
    };

    // Get detailed error enum by message parser state.
    inline http1::error detailed_error(parse_state s) noexcept {
      switch (s) {
      case parse_state::nothing_yet:
        return http1::error::recv_request_timeout_with_nothing;
      case parse_state::start_line:
      case parse_state::expecting_newline:
        return http1::error::recv_request_line_timeout;
      case parse_state::header:
        return http1::error::recv_request_headers_timeout;
      case parse_state::body:
        return http1::error::recv_request_body_timeout;
      case parse_state::completed:
        return http1::error::success;
      }
    }

    // Check the transferred bytes size. If it's zero,
    // then error::end_of_stream will be sent to downstream receiver.
    inline auto check_received_size(std::size_t sz) noexcept {
      return ex::if_then_else(
        sz != 0, //
        ex::just(sz),
        ex::just_error(http1::error::end_of_stream));
    };

    // Initialize state
    template <class Request>
    auto initialize_state(recv_state<Request>& state) noexcept {
      constexpr auto opt = Request::socket_option();
      if (opt.keepalive_timeout != Request::unlimited_timeout) {
        state.remaining_time = opt.keepalive_timeout;
      } else {
        state.remaining_time = opt.total_timeout;
      }
    };

    // recv_request is an customization point object which we names cpo.
    struct recv_request_t {
      template <http1_socket Socket, http1_request Request>
      stdexec::sender auto operator()(Socket socket, Request& req) const noexcept {
        // Type tratis.
        using timepoint = Request::timepoint;
        using recv_state = recv_state<Request>;

        return ex::just(recv_state{.request = req}) //
             | ex::let_value([&](recv_state& state) {
                 // Update necessary information once receive operation completed.
                 auto update_state = [&state](auto start, auto stop, std::size_t recv_size) {
                   state.buffer.commit(recv_size);
                   state.request.update_metric(start, stop, recv_size);
                   state.remaining_time -= std::chrono::duration_cast<std::chrono::seconds>(
                     start - stop);
                 };

                 // Sent error to downstream receiver if receive operation timeout.
                 auto handle_timeout = [&state] noexcept {
                   std::error_code err = detailed_error(state.parser.State());
                   return ex::just_error(err);
                 };

                 // Parse HTTP1 request uses receive buffer.
                 auto parse_request = [&state] {
                   using variant_t = ex::variant_sender<
                     decltype(ex::just(std::declval<bool>())),
                     decltype(ex::just_error(std::declval<std::error_code>()))>;

                   std::error_code ec{};
                   std::size_t parsed_size = state.parser.parse(state.buffer.rbuffer(), ec);
                   if (ec != std::errc{}) {
                     if (ec == http1::error::need_more) {
                       state.buffer.consume(parsed_size);
                       state.buffer.prepare();
                       return variant_t(ex::just(false));
                     }
                     return variant_t(ex::just_error(ec));
                   }
                   return variant_t(ex::just(true));
                 };

                 auto scheduler = socket.context_->get_scheduler();
                 initialize_state(state);
                 return sio::async::read_some(socket, state.buffer.wbuffer()) //
                      | ex::let_value(check_received_size)                    //
                      | ex::timeout(scheduler, state.remaining_time)          //
                      | ex::let_stopped(handle_timeout)                       //
                      | ex::then(update_state)                                //
                      | ex::let_value(parse_request)                          //
                      | ex::repeat_effect_until();
               });
      }
    };
  } // namespace _recv_request

  inline constexpr _recv_request::recv_request_t recv_request{};

  namespace _handle_request {
    struct handle_request_t {
      template <class Request>
      ex::sender auto operator()([[maybe_unused]] const Request& request) const noexcept {
        http1::response response;
        // response.status_code = http1::http_status_code::ok;
        // response.version = request.version;
        return ex::just(response);
      }
    };

  } // namespace _handle_request

  inline constexpr _handle_request::handle_request_t handle_request{};

  namespace _create_response {
    struct create_response_t {
      ex::sender auto operator()(send_state& state) const {
        std::optional<std::string> response_str = state.response.make_response_string();
        if (response_str.has_value()) {
          state.start_line_and_headers = std::move(*response_str);
        }
        return ex::if_then_else(
          response_str.has_value(), ex::just(), ex::just_error(http1::error::invalid_response));
      }
    };
  } // namespace _create_response

  inline constexpr _create_response::create_response_t create_response{};

  namespace _start_server {
    template <http1_request Request>
    bool need_keepalive(const Request& request) noexcept {
      if (request.ContainsHeader(http1::http_header_connection)) {
        return true;
      }
      if (request.Version() == http1::http_version::http11) {
        return true;
      }
      return false;
    }

    template <class E>
    auto handle_error(E&& e) {
      if constexpr (std::is_same_v<E, std::error_code>) {
        std::cout << "Error orrcurred: " << std::forward<E>(e).message() << "\n";
      } else if constexpr (std::is_same_v<E, std::exception_ptr>) {
        std::cout << "Error orrcurred: exception_ptr\n";
      } else {
        std::cout << "Unknown Error orrcurred\n";
      }
    }

    // A http session is a conversation between client and server.
    // We use session id to identify a specific unique conversation.
    template <class Context, class Socket, class Request>
    struct session {
      // Just a simple session id factory which must be replaced in future.
      using id_type = uint64_t;

      static id_type make_session_id() noexcept {
        static std::atomic_int64_t session_idx{0};
        ++session_idx;
        return session_idx.load();
      }

      id_type id{make_session_id()};
      Socket socket{};
      Request request{};
      http1::response response{};
      std::size_t reuse_cnt{0};
    };

    // TODO: how to make ipv6?
    // A http server.
    struct server {
      using context_type = ex::io_uring_context;
      using request_type = client_request;
      using response_type = response;
      using acceptor_handle_type = sio::io_uring::acceptor_handle<sio::ip::tcp>;
      using acceptor_type = sio::io_uring::acceptor<sio::ip::tcp>;
      using socket_type = sio::io_uring::socket_handle<sio::ip::tcp>;
      using session_type = session<context_type, socket_type, request_type>;

      server(std::string_view addr, uint16_t port) noexcept
        : server(sio::ip::endpoint{sio::ip::make_address_v4(addr), port}) {
      }

      server(sio::ip::address addr, uint16_t port) noexcept
        : server(sio::ip::endpoint{addr, port}) {
      }

      explicit server(sio::ip::endpoint endpoint) noexcept
        : endpoint{endpoint}
        , acceptor{&context, sio::ip::tcp::v4(), endpoint} {
      }

      sio::ip::endpoint endpoint{};
      context_type context{};
      acceptor_type acceptor;
    };

    struct start_server_t {
      template <http1_server Server>
      void operator()(Server& server) const noexcept {
        // TODO: some checks must be applied here in the future.
        // e.g. check whether context is able to handle net IO.
        using session_type = Server::session_type;
        using socket_type = Server::socket_type;
        using acceptor_handle_type = Server::acceptor_handle_type;

        auto handles = sio::async::use_resources(
          [&](acceptor_handle_type acceptor) noexcept {
            return sio::async::accept(acceptor) //
                 | sio::let_value_each([&](socket_type socket) {
                     return ex::just(session_type{.socket = socket})               //
                          | ex::let_value([&](session_type& session) {             //
                              return recv_request(session.socket, session.request) //
                                   | ex::upon_error([](auto&&...) noexcept {});
                            });
                   })
                 | sio::ignore_all();
          },
          server.acceptor);
        ex::sync_wait(exec::when_any(handles, server.context.run()));
      }
    };
  } // namespace _start_server

  inline constexpr _start_server::start_server_t start_server{};
  using _start_server::server;

} // namespace net::http1

/*
 * template<net::server Server>
 * net::http::start_server(Server& server)
 *     let_value_with(socket)
 *         return handle_accepetd(socket)
 *                | let_value(create_session(socket))
 *                    return prepare_recv()
 *                           | recv_request(socket)
 *                           | update_metrics()
 *                           | handle_request()
 *                           | send_response()
 *                           | update_metrics()
 *                           | check_keepalive()
 *                           | repeat_effect_until()
 *                           | update_session()
 *                           | handle_error();
 *
 *
 *  start_server(server)                           -> void
 *  handle_accepetd(socket)                        -> void
 *  create_session(socket)                         -> tcp::session
 *  prepare_recv(&session)                         -> void
 *  recv_request(socket, recv_option)              -> sender<request, recv_metrics>
 *  handle_request(&request)                       -> sender<response>
 *  send_response(response, socket, send_option)   -> sender<send_metrics>
 *  update_metrics(&metrics)                       -> void
 *
 *  session: 
 *
 */
