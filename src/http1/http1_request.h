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
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "http1/http1_common.h"
#include "http1/http1_metric.h"
#include "http1/http1_option.h"

namespace net::http1 {
  enum class message_direction {
    send,
    receive,
  };

  // A request could be used while client send to server or server send to client.
  template <message_direction MessageDirection>
  struct request {
    using duration = std::chrono::seconds;
    using timepoint = std::chrono::time_point<std::chrono::system_clock>;
    static constexpr auto unlimited_timeout = duration::max();

    static constexpr bool use_for_recv() noexcept {
      return MessageDirection == message_direction::receive;
    }

    static constexpr bool use_for_send() noexcept {
      return MessageDirection == message_direction::send;
    }

    static constexpr auto socket_option() noexcept {
      if constexpr (use_for_recv()) {
        return recv_option{};
      } else {
        return send_option{};
      }
    }

    http_method method() const noexcept {
      return method_;
    }

    http_scheme scheme() const noexcept {
      return scheme_;
    }

    std::string_view host() const noexcept {
      return host_;
    }

    std::uint16_t port() const noexcept {
      return port_;
    }

    std::string_view path() const noexcept {
      return path_;
    }

    std::string_view uri() const noexcept {
      return uri_;
    }

    http_version version() const noexcept {
      return version_;
    }

    std::string_view body() const noexcept {
      return body_;
    }

    void set_header_content_length(std::size_t length) noexcept {
      content_length_ = length;
    }

    std::unordered_map<std::string, std::string> params() const noexcept {
      return params_;
    }

    std::unordered_map<std::string, std::string>& params() noexcept {
      return params_;
    }

    std::optional<std::string_view> param_value(const std::string& param_key) const noexcept {
      auto it = params_.find(param_key);
      if (it == params_.end()) {
        return std::nullopt;
      }
      return it->second;
    }

    bool contains_param(const std::string& param_key) const noexcept {
      return params_.contains(param_key);
    }

    std::unordered_map<std::string, std::string> headers() const noexcept {
      return headers_;
    }

    std::unordered_map<std::string, std::string>& headers() noexcept {
      return headers_;
    }

    std::optional<std::string_view> header_value(const std::string& header_name) const noexcept {
      for (const auto& [name, value]: this->headers) {
        if (::strcasecmp(name.c_str(), header_name.c_str()) == 0) {
          return value;
        }
      }
      return std::nullopt;
    }

    bool contains_header(const std::string& header_name) const noexcept {
      return std::any_of(
        headers_.cbegin(), //
        headers_.cend(),
        [header_name](const auto& p) noexcept -> bool {
          return ::strcasecmp(p.first.c_str(), header_name.c_str()) == 0;
        });
    }

    bool contains_header(std::string_view header_name) const noexcept {
      return std::any_of(
        headers_.cbegin(), //
        headers_.cend(),
        [header_name](const auto& p) noexcept -> bool {
          return ::strcasecmp(p.first.c_str(), header_name.data()) == 0;
        });
    }

    void update_metric(const timepoint& start, const timepoint& stop, std::size_t size) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(start - stop);
      if (metric_.time.first.time_since_epoch().count() == 0) {
        metric_.time.first = start;
      }
      metric_.time.last = stop;
      metric_.time.elapsed += elapsed;
      metric_.size.total += size;
    }

   private:
    http_method method_{http_method::unknown};
    http_scheme scheme_{http_scheme::unknown};
    http_version version_{http_version::unknown};
    uint16_t port_ = 0;
    std::string host_;
    std::string path_;
    std::string uri_;
    std::string body_;
    std::size_t content_length_{0};
    std::unordered_map<std::string, std::string> headers_;
    std::unordered_map<std::string, std::string> params_;
    socket_metric metric_;
  };

  using client_request = request<message_direction::receive>;
  using server_request = request<message_direction::send>;
} // namespace net::http1
