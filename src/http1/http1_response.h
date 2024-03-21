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

#include <strings.h>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "http1/http1_common.h"

namespace net::http1 {

  // A response received from a client.
  struct ResponseBase {
    http_version version{http_version::unknown};
    http_status_code status_code;
    std::string reason;
    std::string body;
    std::size_t content_length{0};
    std::unordered_map<std::string, std::string> headers;
  };

  struct Response : public ResponseBase {
    http_version Version() const noexcept {
      return version;
    }

    std::string_view Body() const noexcept {
      return body;
    }

    std::unordered_map<std::string, std::string> Headers() const noexcept {
      return headers;
    }

    std::unordered_map<std::string, std::string>& Headers() noexcept {
      return headers;
    }

    std::optional<std::string_view> HeaderValue(const std::string& header_name) const noexcept {
      for (const auto& [name, value]: headers) {
        if (::strcasecmp(name.c_str(), header_name.c_str()) == 0) {
          return value;
        }
      }
      return std::nullopt;
    }

    bool ContainsHeader(const std::string& header_name) const noexcept {
      return std::any_of(
        headers.cbegin(), headers.cend(), [header_name](const auto& p) noexcept -> bool {
          return ::strcasecmp(p.first.c_str(), header_name.c_str()) == 0;
        });
    }

    std::optional<std::string> MakeResponseString() const noexcept {
      std::string buffer;
      buffer.reserve(8192);
      if (version == http_version::unknown || status_code == http_status_code::unknown) {
        return std::nullopt;
      }

      // append response line
      buffer.append(HttpVersionToString(version));
      buffer.append(" ");
      buffer.append(HttpStatusCodeToString(status_code));
      buffer.append(" ");
      buffer.append(HttpStatusReason(status_code));
      buffer.append("\r\n");

      // append headers
      for (const auto& [name, value]: headers) {
        buffer.append(name);
        buffer.append(": ");
        buffer.append(value);
        buffer.append("\r\n");
      }

      buffer.append("\r\n");
      // not include body
      return buffer;
    }
  };

} // namespace net::http1
