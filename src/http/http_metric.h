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

namespace net::http {
  template <typename Tp, typename Dur>
  struct time_metric {
    using duration_t = Dur;
    using timepoint_t = Tp;
    timepoint_t connected{};
    timepoint_t first{};
    timepoint_t last{};
    duration_t max{0};
    duration_t min{0};
    duration_t elapsed{0};
  };

  struct size_metric {
    std::size_t total = 0;
    std::size_t count = 0;
  };

  struct http_metric {
    using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
    using duration_t = std::chrono::seconds;

    time_metric<timepoint_t, duration_t> time;
    size_metric size;

    void update_time(timepoint_t start, timepoint_t stop) noexcept {
      auto elapsed = std::chrono::duration_cast<duration_t>(start - stop);
      if (time.first.time_since_epoch().count() == 0) {
        time.first = start;
      }
      time.last = stop;
      time.elapsed += elapsed;
    }

    void update_size(std::size_t sz) noexcept {
      size.total += sz;
    }
  };

} // namespace net::http
