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

#include <chrono>

namespace net::http {

  template <class Dur>
  struct http_recv_option {
    using duration_t = Dur;
    static constexpr auto unlimited_timeout = duration_t::max();
    duration_t total_timeout{0};
    duration_t keepalive_timeout{0};
  };

  template <class Dur>
  struct http_send_option {
    using duration_t = Dur;
    static constexpr auto unlimited_timeout = duration_t::max();
    duration_t total_timeout{0};
  };

  struct http_option {
    using duration_t = std::chrono::seconds;
    static constexpr auto unlimited_timeout = duration_t::max();

    static constexpr auto recv_option() noexcept {
      return http_recv_option<duration_t>{};
    }

    static constexpr auto send_option() noexcept {
      return http_send_option<duration_t>{};
    }
  };

} // namespace net::http
