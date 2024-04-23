/*
 * Copyright (c) 2024 Xiaoming Zhang
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

#include "stdexec/execution.hpp"

#include "exec/linux/io_uring_context.hpp"

#include "utils/flat_buffer.h"
#include "utils/timeout.h"
#include "utils/if_then_else.h"
#include "utils/just_from_expected.h"
#include "http/v1/http1_server.h"
#include "http/v1/http1_request.h"
#include "http/v1/http1_response.h"
#include "http/http_common.h"
#include "http/http_error.h"
#include "http/http_concept.h"
#include "expected.h"

namespace net::http::http1 {

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

  ex::sender auto parse_request(parser_t& parser, util::flat_buffer<8192>& buffer) noexcept {
    return ex::just_from_expected([&] { return parser.parse(buffer.rbuffer()); })
         | ex::then([&](std::size_t parsed_size) {
             buffer.consume(parsed_size);
             buffer.prepare();
           });
  }

  ex::sender auto recv_request(const tcp_socket& socket) noexcept {
    using state_t =
      std::tuple< http1_client_request, parser_t, util::flat_buffer<8192>, std::chrono::seconds>;

    return ex::just(state_t{}) //
         | ex::let_value([&](state_t& state) {
             auto& [request, parser, buffer, remaining_time] = state;
             parser.reset(&request);
             remaining_time = std::chrono::seconds{120};
             auto scheduler = socket.context_->get_scheduler();

             // Update necessary information once receive operation completed.
             auto update_state = [&](auto start, auto stop, std::size_t recv_size) {
               request.metric.update_time(start, stop);
               request.metric.update_size(recv_size);
               buffer.commit(recv_size);
               remaining_time -= std::chrono::duration_cast<std::chrono::seconds>(start - stop);
             };

             return sio::async::read_some(socket, buffer.wbuffer())              //
                  | ex::let_value(check_received_size)                           //
                  | ex::timeout(scheduler, remaining_time)                       //
                  | ex::stopped_as_error(detailed_error(parser.state()))         //
                  | ex::then(update_state)                                       //
                  | ex::let_value([&] { return parse_request(parser, buffer); }) //
                  | ex::then([&] { return parser.is_completed(); })              //
                  | ex::repeat_effect_until()                                    //
                  | ex::then([&] { return std::move(request); });
           });
  }

  ex::sender auto handle_request(const http1_client_request& request) noexcept {
    http1_client_response response{};
    response.status_code = http_status_code::ok;
    response.version = request.version;
    response.headers = request.headers;
    response.body = request.body;
    return ex::just(std::move(response));
  }

  ex::sender auto
    send_response(const tcp_socket& socket, http1_client_response&& response) noexcept {
    return ex::let_value(ex::just(), [&socket, resp = std::move(response)] mutable {
      return ex::just_from_expected([&resp] { return resp.to_string(); })
           | ex::then([&resp](std::string&& data) { return std::move(data) + resp.body; })
           | ex::let_value([](std::string& data) {
               return ex::just(std::make_tuple(std::as_bytes(std::span(data)), 120s));
             })
           | ex::let_value([&](auto& state) {
               auto& [buffer, remaining_time] = state;
               auto scheduler = socket.context_->get_scheduler();

               // Update necessary information once write operation completed.
               auto update_state = [&](auto start_time, auto stop_time, std::size_t write_size) {
                 resp.metrics.update_time(start_time, stop_time);
                 resp.metrics.update_size(write_size);
                 remaining_time = 120s;
                 assert(write_size <= buffer.size());
                 buffer = buffer.subspan(write_size);
               };
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

  bool need_keepalive(const http1_client_request& request) noexcept {
    if (request.headers.contains(http_header_connection)) {
      return true;
    }
    if (request.version == http_version::http11) {
      return true;
    }
    return false;
  }

  struct handle_error_t {
    auto operator()(auto e) noexcept {
      using E = decltype(e);
      if constexpr (std::same_as<E, std::error_code>) {
        std::cout << "Error orrcurred: " << std::forward<E>(e).message() << "\n";
      } else if constexpr (std::same_as<E, std::exception_ptr>) {
        std::cout << "Error orrcurred: exception_ptr\n";
      } else {
        std::cout << "Unknown Error orrcurred\n";
      }
    }
  };

  inline constexpr handle_error_t handle_error{};

  void start_server(server& s) noexcept {
    auto handles = sio::async::use_resources(
      [&](server::acceptor_handle_type acceptor) noexcept {
        return sio::async::accept(acceptor) //
             | sio::let_value_each([&](server::socket_type socket) {
                 return ex::just(server::session_type{.socket = socket})   //
                      | ex::let_value([&](server::session_type& session) { //
                          return recv_request(session.socket)              //
                               | ex::let_value(handle_request)             //
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
