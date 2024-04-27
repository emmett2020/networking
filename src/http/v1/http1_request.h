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

#include <map>
#include <string>

#include "http/http_common.h"
#include "http/http_concept.h"
#include "http/http_metric.h"
#include "utils/string_compare.h"

namespace net::http::http1 {
  // A request could be used while client send to server or server send to client.
  template <
    http_message_direction Direction,
    http_text_encoding Encoding = http_text_encoding::utf_8 >
  struct request {
    using params_t = std::multimap<std::string, std::string>;
    using headers_t = std::multimap<std::string, std::string, util::case_insensitive_compare>;
    using metric_t = http_metric;

    static constexpr http_message_direction direction() noexcept {
      return Direction;
    }

    static constexpr http_text_encoding text_encoding() noexcept {
      return Encoding;
    }

    http_method method = http_method::unknown;
    http_scheme scheme = http_scheme::unknown;
    http_version version = http_version::unknown;
    port_t port = 0;
    std::string host;
    std::string path;
    std::string uri;
    std::string body;
    std::size_t content_length = 0;
    headers_t headers;
    params_t params;
    http_metric metric;
  };

} // namespace net::http::http1

namespace net::http {
  using http1_client_request = http1::request<http_message_direction::receive_from_client>;
  using http1_server_request = http1::request<http_message_direction::send_to_server>;
}
