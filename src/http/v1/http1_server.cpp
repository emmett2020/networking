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


#include "http/v1/http1_server.h"

namespace net::http::http1 {

  ex::sender auto parse_request(
    message_parser<http1_client_request>& parser,
    util::flat_buffer<8192>& buffer) noexcept {
    return ex::just_from_expected([&] { return parser.parse(buffer.rbuffer()); })
         | ex::then([&](std::size_t parsed_size) {
             buffer.consume(parsed_size);
             buffer.prepare();
           });
  }

  ex::sender auto recv_request(const tcp_socket& socket) noexcept {
    using state_t = std::tuple<
      http1_client_request,
      message_parser<http1_client_request>,
      util::flat_buffer<8192>,
      std::chrono::seconds>;

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

             return sio::async::read_some(socket, buffer.wbuffer())      //
                  | ex::let_value(check_received_size)                   //
                  | ex::timeout(scheduler, remaining_time)               //
                  | ex::stopped_as_error(detailed_error(parser.state())) //
                  | ex::then(update_state)                               //
                  | ex::let_value(parse_request)                         //
                  | ex::repeat_effect_until();
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

  ex::sender auto send_response(const tcp_socket& socket, http1_client_response& resp) noexcept {
    return ex::just_from_expected([&resp] { return resp.to_string(); })
         | ex::then([&resp](std::string& data) { return data + resp.body; })
         | ex::let_value([](std::string& data) {
             return ex::just(std::make_tuple(std::as_bytes(std::span(data)), 120s));
           })
         | ex::let_value([&](auto& state) {
             auto& [buffer, remaining_time] = state;
             auto scheduler = socket.context_->get_scheduler();
             return sio::async::write_some(socket, buffer) //
                  | ex::timeout(scheduler, remaining_time)
                  | ex::stopped_as_error(std::error_code(error::send_timeout))
                  | ex::then([&](auto start_time, auto stop_time, std::size_t write_size) {
                      resp.metrics.update_time(start_time, stop_time);
                      resp.metrics.update_size(write_size);
                      remaining_time = 120s;
                      assert(write_size <= buffer.size());
                      buffer = buffer.subspan(write_size);
                    })
                  | ex::then([] { return buffer.empty(); }) //
                  | ex::repeat_effect_until();
           });
  }

} // namespace net::http::http1
