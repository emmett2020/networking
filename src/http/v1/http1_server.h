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
#include <exception>
#include <tuple>
#include <utility>

#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <exec/linux/io_uring_context.hpp>
#include <exec/timed_scheduler.hpp>
#include <exec/repeat_effect_until.hpp>
#include <sio/io_concepts.hpp>
#include <stdexec/execution.hpp>

#include "http/v1/http1_request.h"
#include "utils/flat_buffer.h"
#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_response.h"
#include "utils/timeout.h"
#include "utils/if_then_else.h"
#include "utils/just_from_expected.h"
#include "http/http_common.h"
#include "http/http_error.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

// TODO: APIs should be constraint by sender_of concept
// TODO: refine headers

namespace net::http::http1 {

  using namespace std::chrono_literals;
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;
  using parser_t = message_parser<http1_client_request>;
  using flat_buffer = util::flat_buffer<65535>; // TODO: dynamic_flat_buffer?

  // A http session is a conversation between client and server.
  // We use session id to identify a specific unique conversation.
  struct http_session {
    // TODO: Just a simple session id factory which must be replaced in future.
    using id_type = uint64_t;

    static id_type make_session_id() noexcept {
      static std::atomic_int64_t session_idx{0};
      ++session_idx;
      return session_idx.load();
    }

    id_type id{make_session_id()};
    tcp_socket socket;
    std::size_t reuse_cnt{0};
  };

  // A http server.
  struct server {
    using context_t = ex::io_uring_context;
    using acceptor_handle_t = sio::io_uring::acceptor_handle<sio::ip::tcp>;
    using acceptor_t = sio::io_uring::acceptor<sio::ip::tcp>;
    using socket_t = sio::io_uring::socket_handle<sio::ip::tcp>;

    server(context_t& ctx, std::string_view addr, port_t port) noexcept
      : server(ctx, sio::ip::endpoint{sio::ip::make_address_v4(addr), port}) {
    }

    server(context_t& ctx, sio::ip::address addr, port_t port) noexcept
      : server(ctx, sio::ip::endpoint{addr, port}) {
    }

    explicit server(context_t& ctx, sio::ip::endpoint endpoint) noexcept
      : endpoint{endpoint}
      , context(ctx)
      , acceptor{&context, sio::ip::tcp::v4(), endpoint} {
    }

    sio::ip::endpoint endpoint{};
    context_t& context; // NOLINT
    acceptor_t acceptor;
  };

  // Get detailed error by message parser state.
  inline http::error detailed_error(http1_parse_state state) noexcept {
    switch (state) {
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

  // Check the transferred bytes size. If it's zero, then error::end_of_stream
  // will be sent to downstream receiver.
  inline auto check_received_size(std::size_t sz) noexcept {
    return ex::if_then_else(
      sz != 0, //
      ex::just(sz),
      ex::just_error(error::end_of_stream));
  };

  // Parse HTTP request uses received flat buffer.
  inline ex::sender auto parse_request(parser_t& parser, flat_buffer& buffer) noexcept {
    return ex::just_from_expected([&] { return parser.parse(buffer.rbuffer()); })
         | ex::then([&](std::size_t parsed_size) {
             buffer.consume(parsed_size);
             buffer.prepare();
           });
  }

  // Receive a completed request from given socket.
  inline ex::sender auto recv_request(const tcp_socket& socket, bool keepalive = false) noexcept {
    using state_t = std::tuple< http1_client_request, parser_t, flat_buffer, http_duration>;

    return ex::just(state_t{}) //
         | ex::let_value([&](state_t& state) {
             auto& [request, parser, buffer, timeout] = state;
             parser.set(&request);
             if (keepalive) {
               timeout = http1_client_request::socket_option().keepalive_timeout;
             } else {
               timeout = http1_client_request::socket_option().total_timeout;
             }
             auto scheduler = socket.context_->get_scheduler();

             // Update necessary information once receive operation completed.
             auto update_state = [&](auto start_time, auto stop_time, std::size_t recv_size) {
               request.metric.update_time(start_time, stop_time);
               request.metric.update_size(recv_size);
               buffer.commit(recv_size);
               timeout -= std::chrono::duration_cast<http_duration>(start_time - stop_time);
             };

             return sio::async::read_some(socket, buffer.wbuffer())              //
                  | ex::let_value(check_received_size)                           //
                  | ex::timeout(scheduler, timeout)                              //
                  | ex::stopped_as_error(detailed_error(parser.state()))         //
                  | ex::then(update_state)                                       //
                  | ex::let_value([&] { return parse_request(parser, buffer); }) //
                  | ex::then([&] { return parser.is_completed(); })              //
                  | ex::repeat_effect_until()                                    //
                  | ex::then([&] { return std::move(request); });
           });
  }

  // Handle http request then request response.
  inline ex::sender auto handle_request(const http1_client_request& request) noexcept {
    http1_client_response response{};
    response.status_code = http_status_code::ok;
    response.version = request.version;
    response.headers = request.headers;
    response.body = request.body;
    return ex::just(std::move(response));
  }

  // Send respopnse to given socket.
  inline ex::sender auto
    send_response(const tcp_socket& socket, http1_client_response&& response) noexcept {
    return ex::let_value(ex::just(), [&socket, resp = std::move(response)] mutable {
      return ex::just_from_expected([&resp] { return resp.to_string(); })
           | ex::then([&resp](std::string&& data) { return std::move(data) + resp.body; })
           | ex::let_value([](std::string& data) { // TODO: does this safe?
               return ex::just(std::make_tuple(std::as_bytes(std::span(data)), 120s));
             })
           | ex::let_value([&](auto& state) {
               auto& [buffer, remaining_time] = state;
               auto scheduler = socket.context_->get_scheduler();

               // Update necessary information once write operation completed.
               auto update_state = [&](auto start_time, auto stop_time, std::size_t write_size) {
                 resp.metrics.update_time(start_time, stop_time);
                 resp.metrics.update_size(write_size);
                 remaining_time = 120s;               // TODO: update remaining_time
                 assert(write_size <= buffer.size()); // TODO: assert?
                 buffer = buffer.subspan(write_size);
               };

               // TODO: should give detailed timeout error message not just send_timeout
               return sio::async::write_some(socket, buffer)                     //
                    | ex::timeout(scheduler, remaining_time)                     //
                    | ex::stopped_as_error(std::error_code(error::send_timeout)) //
                    | ex::then(update_state)                                     //
                    | ex::then([&] { return buffer.empty(); })                   //
                    | ex::repeat_effect_until()                                  //
                    | ex::then([&] { return std::move(resp); });
             });
    });
  }

  inline bool need_keepalive(const http1_client_request& request) noexcept {
    if (request.headers.contains(http_header_connection)) {
      return true;
    }
    if (request.version == http_version::http11) {
      return true;
    }
    return false;
  }

  struct handle_error_t {
    template <typename E>
    auto operator()(E e) noexcept {
      if constexpr (std::same_as<E, std::error_code>) {
        fmt::println("Error: {}", e.message());
      } else if constexpr (std::same_as<E, std::exception_ptr>) {
        try {
          std::rethrow_exception(e);
        } catch (const std::exception& ptr) {
          fmt::println("Error: {}", ptr.what());
        }
      } else {
        fmt::println("Unknown error!");
      }
    }
  };

  inline constexpr handle_error_t handle_error{};

  inline void start_server(server& s) noexcept {
    auto handles = sio::async::use_resources(
      [&](server::acceptor_handle_t acceptor) noexcept {
        return sio::async::accept(acceptor) //
             | sio::let_value_each([&](server::socket_t socket) {
                 return ex::just(http_session{.socket = socket})   //
                      | ex::let_value([&](http_session& session) { //
                          return recv_request(session.socket)      //
                               | ex::let_value(handle_request)     //
                               | ex::let_value([&](http1_client_response& resp) {
                                   return send_response(session.socket, std::move(resp));
                                 })
                               | ex::upon_error(handle_error);
                        });
               })
             | sio::ignore_all();
      },
      s.acceptor);
    ex::sync_wait(exec::when_any(handles, s.context.run()));
  }

} // namespace net::http::http1

namespace net::http {
  using server = net::http::http1::server;
  using http1::start_server;
}
