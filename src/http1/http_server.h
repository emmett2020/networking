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

#include <array>
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
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_response.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

namespace net::http1 {
  struct send_state {
    std::string start_line_and_headers;
    http1::Response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
  };

  namespace _recv_request {
    template <http_request Request>
    struct recv_state {
      Request request{};
      MessageParser<Request> parser{&request};
      util::flat_buffer<8192> buffer{};
      Request::duration remaining_time{0};
    };

    template <class Request>
    void initialize_state(recv_state<Request>& state) {
      // TODO(xiaoming): add concept limit
      constexpr auto opt = Request::socket_option();
      if (opt.keepalive_timeout != Request::unlimited_timeout) {
        state.remaining_time = opt.keepalive_timeout;
      } else {
        state.remaining_time = opt.total_timeout;
      }
    }

    template <class Request>
    void update_state(auto elapsed, std::size_t recv_size, recv_state<Request>& state) noexcept {
      (void) elapsed;
      // state.remaining_time -= elapsed.count(); // BUGBUG
    }

    // Get detailed error enum by message parser state.
    inline Error detailed_error(http1::RequestParser::MessageState state) noexcept {
      using State = http1::RequestParser::MessageState;
      switch (state) {
      case State::kNothingYet:
        return http1::Error::kRecvRequestTimeoutWithNothing;
      case State::kStartLine:
      case State::kExpectingNewline:
        return http1::Error::kRecvRequestLineTimeout;
      case State::kHeader:
        return http1::Error::kRecvRequestHeadersTimeout;
      case State::kBody:
        return http1::Error::kRecvRequestBodyTimeout;
      case State::kCompleted:
        return http1::Error::kSuccess;
      }
    }

    inline ex::sender auto check_recv_size(std::size_t recv_size) {
      return ex::if_then_else(
        recv_size != 0, //
        ex::just(recv_size),
        ex::just_error(http1::Error::kEndOfStream));
    }

    // recv_request is an customization point object which we names cpo.
    struct recv_request_t {
      template <http1_socket Socket, http_request Request>
      stdexec::sender auto operator()(Socket socket, Request& req) const noexcept {
        // Type tratis.
        using timepoint = Request::timepoint;
        using recv_state = recv_state<Request>;

        auto scheduler = socket.context_->get_scheduler();
        using timepoint_of_scheduler = exec::time_point_of_t<decltype(scheduler)>;

        return ex::just(recv_state{.request = req}) //
             | ex::let_value([&](recv_state& state) {
                 initialize_state(state);
                 return sio::async::read_some(socket, state.buffer.writable_buffer()) //
                      | ex::let_value(check_recv_size)                                //
                      | ex::timeout(scheduler, state.remaining_time)                  //
                      | ex::let_stopped([&] {
                          auto err = detailed_error(state.parser.State());
                          return ex::just_error(err);
                        })
                      | ex::then([&](auto start, auto stop, std::size_t recv_size) {
                          state.buffer.commit(recv_size);
                          state.request.update_metric(start, stop, recv_size);
                          update_state(stop - start, recv_size, state);
                        })
                      | ex::let_value([&] {
                          std::error_code ec{};
                          std::size_t parsed_size = state.parser.Parse(state.buffer.data(), ec);
                          if (ec) {
                            if (ec == http1::Error::kNeedMore) {
                              state.buffer.consume(parsed_size);
                              state.buffer.prepare();
                              return ex::just(false);
                            }
                            return ex::just_error(ec && ec != http1::Error::kNeedMore);
                          }
                          // parse done
                          return ex::just(true);
                        })
                      | ex::repeat_effect_until();
               });
      }
    };
  } // namespace _recv_request

  inline constexpr _recv_request::recv_request_t recv_request{};

  namespace _handle_request {
    struct handle_request_t {
      template <class Request>
      ex::sender auto operator()(const Request& request) const noexcept {
        http1::Response response;
        response.status_code = http1::HttpStatusCode::kOK;
        response.version = request.version;
        return ex::just(response);
      }
    };

  } // namespace _handle_request

  inline constexpr _handle_request::handle_request_t handle_request{};

  namespace _create_response {
    struct create_response_t {
      ex::sender auto operator()(send_state& state) const {
        std::optional<std::string> response_str = state.response.MakeResponseString();
        if (response_str.has_value()) {
          state.start_line_and_headers = std::move(*response_str);
        }
        return ex::if_then_else(
          response_str.has_value(), ex::just(), ex::just_error(http1::Error::kInvalidResponse));
      }
    };
  } // namespace _create_response

  inline constexpr _create_response::create_response_t create_response{};

  namespace _start_server {
    template <http_request Request>
    bool need_keepalive(const Request& request) noexcept {
      if (request.ContainsHeader(http1::kHttpHeaderConnection)) {
        return true;
      }
      if (request.Version() == http1::HttpVersion::kHttp11) {
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
      Response response{};
      std::size_t reuse_cnt{0};
    };

    // TODO: how to make ipv6?
    // A http server.
    struct server {
      using context_type = ex::io_uring_context;
      using request_type = client_request;
      using response_type = Response;
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
      template <http_server Server>
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
