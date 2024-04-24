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

#pragma once

#include <chrono>
#include <tuple>
#include <utility>

#include "utils/execution.h"

#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_request.h"
#include "http/http_error.h"
#include "utils/flat_buffer.h"
#include "utils/timeout.h"
#include "utils/if_then_else.h"
#include "utils/just_from_expected.h"

namespace net::http::http1 {
  using namespace std::chrono_literals;
  using parser_t = message_parser<http1_client_request>;
  using flat_buffer = util::flat_buffer<65535>; // TODO: dynamic_flat_buffer?
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;

  // Get detailed error by message parser state.
  inline std::error_code detailed_error(const parser_t& parser) noexcept {
    switch (parser.state()) {
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
               timeout = 10s; // DEBUG
             } else {
               timeout = http1_client_request::socket_option().total_timeout;
               timeout = 5s; // DEBUG
             }
             auto scheduler = socket.context_->get_scheduler();

             // Update necessary information once receive operation completed.
             auto update_state = [&](auto start_time, auto stop_time, std::size_t recv_size) {
               request.metric.update_time(start_time, stop_time);
               request.metric.update_size(recv_size);
               buffer.commit(recv_size);
               timeout -= std::chrono::duration_cast<http_duration>(start_time - stop_time);
             };

             return sio::async::read_some(socket, buffer.wbuffer())                         //
                  | ex::let_value(check_received_size)                                      //
                  | ex::timeout(scheduler, timeout)                                         //
                  | ex::let_stopped([&] { return ex::just_error(detailed_error(parser)); }) //
                  | ex::then(update_state)                                                  //
                  | ex::let_value([&] { return parse_request(parser, buffer); })            //
                  | ex::then([&] { return parser.is_completed(); })                         //
                  | ex::repeat_effect_until()                                               //
                  | ex::then([&] { return std::move(request); });
           });
  }

} // namespace net::http::http1
