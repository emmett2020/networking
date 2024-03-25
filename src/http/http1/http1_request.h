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

#include <strings.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "http/http_common.h"
#include "http/http_concept.h"
#include "http/http_metric.h"
#include "http/http_option.h"

namespace net::http::http1 {
  // A request could be used while client send to server or server send to client.
  template <
    http_message_direction Direction,
    http_text_encoding Encoding = http_text_encoding::utf_8,
    http1_metric_concept Metric = http_metric,
    http1_option_concept Option = http_option>
  struct request {
    using param_t = std::unordered_map<std::string, std::string>;
    using header_t = std::unordered_map<std::string, std::string>;

    static constexpr http_message_direction direction() noexcept {
      return Direction;
    }

    static constexpr auto socket_option() noexcept {
      if constexpr (direction() == http_message_direction::receive_from_client) {
        return Option::recv_option();
      } else {
        return Option::send_option();
      }
    }

    static constexpr http_text_encoding text_encoding() noexcept {
      return Encoding;
    }

    http_method method() const noexcept {
      return method_;
    }

    http_scheme scheme() const noexcept {
      return scheme_;
    }

    const std::string& host() const noexcept {
      return host_;
    }

    std::string& host() noexcept {
      return host_;
    }

    std::uint16_t port() const noexcept {
      return port_;
    }

    const std::string& path() const noexcept {
      return path_;
    }

    std::string& path() noexcept {
      return path_;
    }

    const std::string& uri() const noexcept {
      return uri_;
    }

    std::string& uri() noexcept {
      return uri_;
    }

    http_version version() const noexcept {
      return version_;
    }

    const std::string& body() const noexcept {
      return body_;
    }

    std::string& body() noexcept {
      return body_;
    }

    const param_t& params() const noexcept {
      return params_;
    }

    param_t& params() noexcept {
      return params_;
    }

    const header_t& headers() const noexcept {
      return headers_;
    }

    header_t& headers() noexcept {
      return headers_;
    }

    bool contain_param(const std::string& name) const noexcept {
      return params_.contains(name);
    }

    bool contain_header(std::string_view name) const noexcept {
      return std::any_of(
        headers_.cbegin(), //
        headers_.cend(),
        [name](const auto& cur) noexcept { //
          return ::strcasecmp(cur.c_str(), name.data()) == 0;
        });
    }

    std::optional<std::reference_wrapper<const std::string>>
      param_value(const std::string& name) const noexcept {
      if (const auto it = params_.find(name); it != params_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<std::string>>
      param_value(const std::string& name) noexcept {
      if (auto it = params_.find(name); it != params_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<const std::string>>
      header_value(const std::string& name) const noexcept {
      for (const auto& [n, v]: headers_) {
        if (::strcasecmp(n.c_str(), name.c_str()) == 0) {
          return v;
        }
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<std::string>>
      header_value(const std::string& name) noexcept {
      for (auto& [n, v]: headers_) {
        if (::strcasecmp(n.c_str(), name.c_str()) == 0) {
          return v;
        }
      }
      return std::nullopt;
    }

    void update_metric(
      const Metric::timepoint_t& start,
      const Metric::timepoint_t& stop,
      std::size_t size) noexcept {
      metric_.update_time(start, stop);
      metric_.update_size(size);
    }

   private:
    http_method method_ = http_method::unknown;
    http_scheme scheme_ = http_scheme::unknown;
    http_version version_ = http_version::unknown;

    uint16_t port_ = 0;
    std::string host_;
    std::string path_;
    std::string uri_;
    std::string body_;
    std::size_t content_length_{0};

    header_t headers_;
    param_t params_;
    Metric metric_;
  };

  using http1_client_request = request<http_message_direction::receive_from_client>;
  using http1_server_request = request<http_message_direction::send_to_server>;
} // namespace net::http::http1
