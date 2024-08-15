/*
 * Copyright (c) 2024 Xiaoming Zhang
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions (the "License"); you may not use this file except in compliance with
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

#include <cstring>
#include <string>
#include <map>

#include "net/http/http_common.h"
#include "net/http/http_metric.h"
#include "net/utils/string_compare.h"

namespace net::http {
  struct http_response {
    using headers_t = std::multimap<std::string, std::string, utils::case_insensitive_compare>;
    using metric_t = http_metric;

    static constexpr http_text_encoding text_encoding() noexcept {
      return http_text_encoding::utf_8;
    }

    http_version version = http_version::unknown;
    http_status_code status_code = http_status_code::unknown;
    std::string reason;
    std::string body;
    std::size_t content_length = 0;
    headers_t headers;
    metric_t metric;
    bool need_keepalive = false;
  };


} // namespace net::http
