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

#include <stdatomic.h>
#include <chrono>

#include "http/http_time.h"

namespace net::http {
  struct time_metric {
    http_timepoint connected;
    http_timepoint first;
    http_timepoint last;
    http_duration max{0};
    http_duration min{0};
    http_duration elapsed{0};
  };

  struct size_metric {
    std::size_t total = 0;
    std::size_t count = 0;
  };

  struct http_metric {
    time_metric time;
    size_metric size;

    void update_time(http_timepoint start, http_timepoint stop) noexcept {
      auto elapsed = std::chrono::duration_cast<http_duration>(start - stop);
      if (time.first.time_since_epoch().count() == 0) {
        time.first = start;
      }
      time.last = stop;
      time.elapsed += elapsed;
    }

    void update_size(std::size_t sz) noexcept {
      size.total += sz;
      size.count += 1;
    }
  };

  struct server_metric {
    atomic_size_t total_recv_size = 0;
    atomic_size_t total_write_size = 0;
  };

} // namespace net::http
