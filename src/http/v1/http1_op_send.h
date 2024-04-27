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

#include "utils/execution.h"
#include "http/v1/http1_response.h"
#include "http/http_option.h"
#include "utils/just_from_expected.h"
#include "utils/timeout.h"

namespace net::http::http1 {

  using namespace std::chrono_literals;
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;

  // Send respopnse to given socket.
  inline ex::sender auto send_response(
    const tcp_socket& socket,
    const http_option& option,
    http_response&& response) noexcept {
    return ex::let_value(ex::just(std::move(response)), [&](http_response& resp) {
      return ex::just_from_expected([&] { return resp.to_string(); })
           | ex::let_value([&](std::string& data) {
               data += resp.body;
               auto timeout = option.total_send_timeout;
               return ex::just(std::make_tuple(std::as_bytes(std::span(data)), timeout))
                    | ex::let_value([&](auto& state) {
                        auto& [buffer, timeout] = state;
                        auto scheduler = socket.context_->get_scheduler();

                        // Update necessary information once write operation completed.
                        auto update_state =
                          [&](auto start_time, auto stop_time, std::size_t write_size) {
                            resp.metric.update_time(start_time, stop_time);
                            resp.metric.update_size(write_size);
                            timeout -= std::chrono::duration_cast<http_duration>(
                              start_time - stop_time);
                            assert(write_size <= buffer.size());
                            buffer = buffer.subspan(write_size);
                          };

                        return sio::async::write_some(socket, buffer)                     //
                             | ex::timeout(scheduler, timeout)                            //
                             | ex::stopped_as_error(std::error_code(error::send_timeout)) //
                             | ex::then(update_state)                                     //
                             | ex::then([&] { return buffer.empty(); })                   //
                             | ex::repeat_effect_until()                                  //
                             | ex::then([&] { return std::move(resp); });
                      });
             });
    });
  }

} // namespace net::http::http1
