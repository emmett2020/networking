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

#include <system_error>

#include "net/http/http_common.h"
#include "net/http/http_time.h"
#include "net/http/http_response.h"
#include "net/http/http_option.h"
#include "net/http/http_server.h"
#include "net/http/v1/http1_op_recv.h"

namespace net::http::http1 {

  using namespace std::chrono_literals;
  namespace ex = stdexec;

  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;

  using valid_ret_t = exec::variant_sender<
    decltype(ex::just_error(std::declval<std::error_code>())),
    decltype(ex::just(std::declval<http_connection>()))>;

  // should return variant_sender
  inline valid_ret_t valid_response(http_connection& conn) noexcept {
    const http_response& rsp = conn.response;
    if (rsp.status_code == http_status_code::unknown) {
      return ex::just_error(std::error_code(error::invalid_response));
    }
    if (rsp.version == http_version::unknown) {
      return ex::just_error(std::error_code(error::invalid_response));
    }
    return ex::just(std::move(conn));
  }

  inline void fill_response_buffer(const http_response& rsp, flat_buffer& buffer) noexcept {
    // Append response line.
    if (rsp.version == net::http::http_version::http10) {
      std::string_view version = to_http_version_string(rsp.version);
      std::string_view code = to_http_status_code_string(rsp.status_code);
      std::string_view reason = to_http_status_reason(rsp.status_code);
      buffer.write(version);
      buffer.write(" ");
      buffer.write(code);
      buffer.write(" ");
      buffer.write(reason);
      buffer.write("\r\n");
    } else {
      buffer.write(to_http1_response_line(rsp.status_code));
      buffer.write("\r\n");
    }

    // Append headers.
    for (const auto& [name, value]: rsp.headers) {
      buffer.write(name);
      buffer.write(": ");
      buffer.write(value);
      buffer.write("\r\n");
    }

    buffer.write("\r\n");
    // Append body.
    buffer.write(rsp.body);
  }

  // Send respopnse to given socket.
  inline ex::sender auto send_response(http_connection& conn) noexcept {
    fill_response_buffer(conn.response, conn.buffer);
    auto timeout = conn.option.total_send_timeout;
    return ex::just(std::move(conn), conn.buffer.rbuffer(), timeout) //
         | ex::let_value([](http_connection& conn, auto buffer, http_duration& timeout) {
             auto scheduler = conn.socket.context_->get_scheduler();

             // Update necessary information once write operation completed.
             auto update_state = [&](auto start_time, auto stop_time, std::size_t write_size) {
               conn.response.metric.update_time(start_time, stop_time);
               conn.response.metric.update_size(write_size);
               timeout -= std::chrono::duration_cast<http_duration>(start_time - stop_time);
               assert(write_size <= buffer.size());
               buffer = buffer.subspan(write_size);
             };

             return sio::async::write_some(conn.socket, buffer)                //
                  | net::utils::timeout(scheduler, timeout)                    //
                  | ex::stopped_as_error(std::error_code(error::send_timeout)) //
                  | ex::then(update_state)                                     //
                  | ex::then([&] { return buffer.empty(); })                   //
                  | exec::repeat_effect_until()                                //
                  | ex::then([&] { return std::move(conn); });
           });
  }

} // namespace net::http::http1
