/*
 * Copyright (c) 2023 Xiaoming Zhang
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

#include "expected.h"
#include "http/http_common.h"
#include "http/http_concept.h"
#include "http/http_error.h"
#include "http/http_metric.h"
#include "http/http_option.h"
#include "utils/string_compare.h"

namespace net::http::http1 {
  template <
    http_message_direction Direction,
    http_text_encoding Encoding = http_text_encoding::utf_8,
    http1_metric_concept Metric = http_metric,
    http1_option_concept Option = http_option>
  struct response {
    using headers_t = std::multimap<std::string, std::string, util::case_insensitive_compare>;
    using metric_t = Metric;
    using option_t = Option;

    static constexpr http_message_direction direction() noexcept {
      return Direction;
    }

    static constexpr http_text_encoding text_encoding() noexcept {
      return Encoding;
    }

    static constexpr auto socket_option() noexcept {
      if constexpr (direction() == http_message_direction::receiver_from_server) {
        return option_t::recv_option();
      } else {
        return option_t::send_option();
      }
    }

    [[nodiscard]] string_expected to_string() const noexcept {
      std::string buffer;
      buffer.reserve(65535);
      if (status_code == http_status_code::unknown) {
        return net::unexpected(make_error_code(error::unknown_status));
      }
      if (version == http_version::unknown) {
        return net::unexpected(make_error_code(error::bad_version));
      }

      // Append response line.
      buffer.append(to_http_version_string(version));
      buffer.append(" ");
      buffer.append(to_http_status_code_string(status_code));
      buffer.append(" ");
      buffer.append(to_http_status_reason(status_code));
      buffer.append("\r\n");

      // Append headers.
      for (const auto& [name, value]: headers) {
        buffer.append(name);
        buffer.append(": ");
        buffer.append(value);
        buffer.append("\r\n");
      }
      buffer.append("\r\n");

      // Not include body
      return buffer;
    }

    http_version version = http_version::unknown;
    http_status_code status_code = http_status_code::unknown;
    std::string reason;
    std::string body;
    std::size_t content_length = 0;
    headers_t headers;
    http_metric metrics{};
  };

} // namespace net::http::http1

namespace net::http {
  using http1_client_response = http1::response<http_message_direction::send_to_client>;
  using http1_server_response = http1::response<http_message_direction::receiver_from_server>;
}
