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
#include <span>
#include <string>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>

#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <exec/linux/io_uring_context.hpp>
#include <exec/timed_scheduler.hpp>
#include <exec/repeat_effect_until.hpp>

#include "expected.h"
#include "http/v1/http1_request.h"
#include "sio/io_concepts.hpp"
#include "stdexec/execution.hpp"
#include "utils/timeout.h"
#include "utils/if_then_else.h"
#include "utils/flat_buffer.h"
#include "utils/just_from_expected.h"
#include "http/http_common.h"
#include "http/http_error.h"
#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_response.h"
#include "http/http_concept.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

namespace net::http::http1 {

  using namespace std::chrono_literals;
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;
  using parser_t = message_parser<http1_client_request>;

  // Get detailed error enum by message parser state.
  inline http::error detailed_error(http1_parse_state s) noexcept {
    switch (s) {
    case http1_parse_state::nothing_yet:
      return error::recv_request_timeout_with_nothing;
    case http1_parse_state::start_line:
    case http1_parse_state::expecting_newline:
      return error::recv_request_line_timeout;
    case http1_parse_state::header:
      return error::recv_request_headers_timeout;
    case http1_parse_state::body:
      return error::recv_request_body_timeout;
    case http1_parse_state::completed:
      return error::success;
    }
  }

  // Check the transferred bytes size. If it's zero,
  // then error::end_of_stream will be sent to downstream receiver.
  inline auto check_received_size(std::size_t sz) noexcept {
    return ex::if_then_else(
      sz != 0, //
      ex::just(sz),
      ex::just_error(error::end_of_stream));
  };

  // Parse HTTP request uses receive buffer.
  ex::sender auto parse_request(parser_t& parser, util::flat_buffer<8192>& buffer) noexcept;

  ex::sender auto recv_request(const tcp_socket& socket) noexcept;

  ex::sender auto handle_request(const http1_client_request& request) noexcept;

  ex::sender auto send_response(const tcp_socket& socket, http1_client_response& resp) noexcept;

  namespace _start_server {
    template <http1_request_concept Request>
    bool need_keepalive(const Request& request) noexcept {
      if (request.headers.contains(http_header_connection)) {
        return true;
      }
      if (request.version == http_version::http11) {
        return true;
      }
      return false;
    }

    template <class E>
    void handle_error(E&& e) noexcept {
      if constexpr (std::same_as<E, std::error_code>) {
        std::cout << "Error orrcurred: " << std::forward<E>(e).message() << "\n";
      } else if constexpr (std::same_as<E, std::exception_ptr>) {
        std::cout << "Error orrcurred: exception_ptr\n";
      } else {
        std::cout << "Unknown Error orrcurred\n";
      }
    }

    // A http session is a conversation between client and server.
    // We use session id to identify a specific unique conversation.
    template <class Context, class Socket, class Request, class Response>
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
      using request_type = http1_client_request;
      using response_type = http1_client_response;
      using acceptor_handle_type = sio::io_uring::acceptor_handle<sio::ip::tcp>;
      using acceptor_type = sio::io_uring::acceptor<sio::ip::tcp>;
      using socket_type = sio::io_uring::socket_handle<sio::ip::tcp>;
      using session_type = session<context_type, socket_type, request_type, response_type>;

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
      template <http1_server_concept Server>
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
                                   | ex::let_value([&] {
                                       return handle_request(session.request, session.response);
                                     })
                                   | ex::let_value([&] {
                                       return send_response(session.socket, session.response);
                                     })
                                   | ex::upon_error([]<class E>(E&& e) {
                                       if constexpr (std::same_as<E, std::error_code>) {
                                         fmt::println("Error: {}", std::forward<E>(e).message());
                                       }
                                     });
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

} // namespace net::http::http1

namespace net::http {
  using server = net::http::http1::server;
  using http1::start_server;
}

// 1. How to act as a cpo since we don't know the request type?
// 2. How to use request after sent response ? Or, should we use request after sent response?
// 3. Hard to coding ?
// 4. Not easy to extend?

/*
 * template<net::server Server>
 * net::http::start_server(Server& server)
 *     let_value_with(socket)
 *         return handle_accepetd(socket)
 *                | let_value(create_session(socket))
 *                    return recv_request(socket)
 *                           | update_metrics()
 *                           | handle_request()
 *                           | send_response(socket)
 *                           | update_metrics()
 *                           | check_keepalive()
 *                           | repeat_effect_until()
 *                           | update_session()
 *                           | handle_error();
 *
 *
          return handle_accepetd(socket)
               | let_value(create_session(socket))
                   return prepare_recv()
                          | recv_request(socket, session.request)
                          | ex::then([] { return update_metrics(session.request.metrics); })
                          | ex::let_value([] { return handle_request(session.request, session.response); })
                          | ex::let_value([] { return send_response(socket, session.response); })
                          | ex::then([] { return update_metrics(session.response.metrics); })
                          | ex::then([] { return check_keepalive(session.request); })
                          | ex::repeat_effect_until()
                          | ex::upon_error(handle_error);

 *  start_server(server)                           -> void
 *  handle_accepetd(socket)                        -> void
 *  create_session(socket)                         -> tcp::session
 *  prepare_recv(&session)                         -> void
 *  recv_request(socket, &request)                 -> sender<void>
 *  handle_request(&request, &response)            -> sender<void>
 *  send_response(socket, response)                -> sender<void>
 *  update_metrics(&metrics)                       -> void
 *  check_keepalive(&request)                      -> sender<bool>
 *
 *
 */
