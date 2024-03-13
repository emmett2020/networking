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

#include <cstdint>
#include <chrono>

namespace net::http1 {
  struct time_metric {
    using duration = std::chrono::seconds;
    using timepoint = std::chrono::time_point<std::chrono::system_clock>;
    timepoint connected{};
    timepoint first{};
    timepoint last{};
    duration max{0};
    duration min{0};
    duration elapsed{0};
  };

  struct size_metric {
    uint64_t total{0};
    uint32_t count{0};
  };

  struct socket_metric {
    time_metric time;
    size_metric size;
  };

  struct recv_metric {
    time_metric time;
    size_metric size;
  };

  struct send_metric {
    time_metric time;
    size_metric size;
  };

  struct server_metric {
    recv_metric recv;
    send_metric send;
  };

} // namespace net::http1
