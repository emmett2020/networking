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

  struct response {
    http_version version() const noexcept {
      return version_;
    }

    std::string_view body() const noexcept {
      return body_;
    }

    std::unordered_map<std::string, std::string> headers() const noexcept {
      return headers_;
    }

    std::unordered_map<std::string, std::string>& headers() noexcept {
      return headers_;
    }

    std::optional<std::string_view> header_value(const std::string& header_name) const noexcept {
      for (const auto& [name, value]: headers_) {
        if (::strcasecmp(name.c_str(), header_name.c_str()) == 0) {
          return value;
        }
      }
      return std::nullopt;
    }

    bool contains_header(const std::string& header_name) const noexcept {
      return std::any_of(
        headers_.cbegin(), headers_.cend(), [header_name](const auto& p) noexcept -> bool {
          return ::strcasecmp(p.first.c_str(), header_name.c_str()) == 0;
        });
    }

    std::optional<std::string> make_response_string() const noexcept {
      std::string buffer;
      buffer.reserve(8192);
      if (version_ == http_version::unknown || status_code_ == http_status_code::unknown) {
        return std::nullopt;
      }

      // append response line
      buffer.append(http_version_to_string(version_));
      buffer.append(" ");
      buffer.append(http_status_code_to_string(status_code_));
      buffer.append(" ");
      buffer.append(http_status_reason(status_code_));
      buffer.append("\r\n");

      // append headers
      for (const auto& [name, value]: headers_) {
        buffer.append(name);
        buffer.append(": ");
        buffer.append(value);
        buffer.append("\r\n");
      }

      buffer.append("\r\n");
      // not include body
      return buffer;
    }

   private:
    http_version version_{http_version::unknown};
    http_status_code status_code_;
    std::string reason_;
    std::string body_;
    std::size_t content_length_{0};
    std::unordered_map<std::string, std::string> headers_;
  };

} // namespace net::http1
