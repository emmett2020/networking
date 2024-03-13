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

#include <stdexec/execution.hpp>
#include <exec/repeat_effect_until.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>

#include "http1/http_concept.h"
#include "http1/http_request.h"
#include "http1/http_message_parser.h"
#include "utils/timeout.h"
#include "utils/if_then_else.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

namespace net::http1 {

  template <http_request Request>
  struct recv_state {
    Request request{};
    http1::RequestParser parser{&request};
    std::array<std::byte, 8192> buffer{};
    uint32_t unparsed_size{0};
    Request::duration remaining_time{0};
  };

  inline ex::sender auto check_recv_size(std::size_t recv_size) {
    return ex::if_then_else(
      recv_size != 0, ex::just(recv_size), ex::just_error(http1::Error::kEndOfStream));
  }

  struct recv_request_t {
    template <http_request Request>
    stdexec::sender auto operator()(Request& req) const noexcept {
      using timepoint = Request::timepoint;
      auto socket = req.socket;
      auto scheduler = socket.context_->get_scheduler();

      return ex::just(recv_state<Request>{.request = req}) //
           | ex::let_value([&](recv_state<Request>& state) {
               auto recv_buffer = [&] {
                 return std::span{state.buffer}.subspan(state.unparsed_size);
               };

               initialize_state(state);
               return sio::async::read_some(socket, recv_buffer())                               //
                    | ex::let_value(check_recv_size)                                             //
                    | ex::timeout(scheduler, state.remaining_time)                               //
                    | ex::let_stopped([] { return ex::just_error(http1::Error::kRecvTimeout); }) //
                    | ex::then([&](timepoint start, timepoint stop, std::size_t recv_size) {
                        state.request.update_metric(start, stop, recv_size);
                        update_state(stop - start, recv_size, state);
                      })                                                  //
                    | ex::let_value([&] { return parse_request(state); }) //
                    | ex::repeat_effect_until()                           //
                    | ex::then([&] { return std::move(state.request); });
             });
    }
  };

  inline constexpr recv_request_t recv_request{};
} // namespace net::http1
