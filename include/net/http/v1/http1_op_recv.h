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
#include <utility>

#include <stdexec/execution.hpp>
#include <exec/repeat_effect_until.hpp>

#include "net/http/http_option.h"
#include "net/http/http_time.h"
#include "net/http/http_request.h"
#include "net/http/http_error.h"
#include "net/http/v1/http1_message_parser.h"
#include "net/http/v1/http_connection.h"
#include "net/utils/flat_buffer.h"
#include "net/utils/timeout.h"
#include "net/utils/if_then_else.h"
#include "net/utils/just_from_expected.h"

// TODO: so many inline functions?

namespace net::http::http1 {
  using namespace std::chrono_literals;
  namespace ex = stdexec;

  using parser_t = message_parser<http_request>;
  using flat_buffer = utils::flat_buffer<65535>; // TODO: dynamic_flat_buffer?
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
  inline stdexec::sender auto check_received_size(std::size_t received_size) noexcept {
    return utils::if_then_else(
      received_size != 0, //
      ex::just(received_size),
      ex::just_error(std::error_code(error::end_of_stream)));
  };

  inline http_duration infer_timeout(const http_connection& conn) noexcept {
    if (conn.option.need_keepalive) {
      return conn.option.keepalive_timeout;
    }
    return conn.option.total_recv_timeout;
  }

  // Parse HTTP request uses received flat buffer.
  inline ex::sender auto parse_request(parser_t& parser, flat_buffer& buffer) noexcept {
    return net::utils::just_from_expected([&] { return parser.parse(buffer.rbuffer()); })
         | ex::then([&](std::size_t parsed_size) {
             buffer.consume(parsed_size);
             buffer.prepare();
           });
  }

  // Receive a completed request from given socket.
  inline ex::sender auto recv_request(http_connection&& conn) noexcept {
    return ex::just(std::move(conn), parser_t{}, infer_timeout(conn))
         | ex::let_value([](http_connection& conn, auto& parser, http_duration& timeout) {
             parser.set(&conn.request);
             auto scheduler = conn.socket.context_->get_scheduler();

             auto update_state = [&](auto start_time, auto stop_time, std::size_t recv_size) {
               conn.request.metric.update_time(start_time, stop_time);
               conn.request.metric.update_size(recv_size);
               conn.buffer.commit(recv_size);
               timeout -= std::chrono::duration_cast<http_duration>(start_time - stop_time);
             };


             return sio::async::read_some(conn.socket, conn.buffer.wbuffer())               //
                  | ex::let_value(check_received_size)                                      //
                  | net::utils::timeout(scheduler, timeout)                                 //
                  | ex::let_stopped([&] { return ex::just_error(detailed_error(parser)); }) //
                  | ex::then(update_state)                                                  //
                  | ex::let_value([&] { return parse_request(parser, conn.buffer); })       //
                  | ex::then([&] { return parser.is_completed(); })                         //
                  | exec::repeat_effect_until()                                             //
                  | ex::then([&] { return std::move(conn); });
           });
  }

} // namespace net::http::http1
