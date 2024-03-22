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
  using namespace net::http::common; // NOLINT
  enum class message_direction {
    send_to_server,
    receive_from_client,
  };

  // A request could be used while client send to server or server send to client.
  template <message_direction MessageDirection>
  struct request {
    using duration_t = std::chrono::seconds;
    using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
    static constexpr auto unlimited_timeout = duration_t::max();

    static constexpr message_direction direction() noexcept {
      return MessageDirection;
    }

    static constexpr auto socket_option() noexcept {
      if constexpr (direction() == message_direction::receive_from_client) {
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

    void set_content_length(std::size_t length) noexcept {
      content_length_ = length;
    }

    std::unordered_map<std::string, std::string> params() const noexcept {
      return params_;
    }

    std::unordered_map<std::string, std::string>& params() noexcept {
      return params_;
    }

    std::optional<std::string_view> param_value(const std::string& name) const noexcept {
      // TODO: use std::ranges?
      auto it = params_.find(name);
      if (it == params_.end()) {
        return std::nullopt;
      }
      return it->second;
    }

    bool contains_param(const std::string_view& name) const noexcept {
      return params_.contains(std::string(name));
    }

    // bool contains_param(const std::string& name) const noexcept {
    //   return params_.contains(name);
    // }

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

    void update_metric(const timepoint_t& start, const timepoint_t& stop, std::size_t size) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(start - stop);
      if (metric_.time.first.time_since_epoch().count() == 0) {
        metric_.time.first = start;
      }
      metric_.time.last = stop;
      metric_.time.elapsed += elapsed;
      metric_.size.total += size;
    }

   private:
    // TODO: Should we change std::string and std::unordered_map to thirdparits'
    // version in order to get a better performance ?
    // I don't want something allocated at heap...,
    // We may reference boost::flat_map which also be introduced at cpp23.
    // https://stackoverflow.com/questions/21166675/boostflat-map-and-its-performance-compared-to-map-and-unordered-map
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

  using client_request = request<message_direction::receive_from_client>;
  using server_request = request<message_direction::send_to_server>;
} // namespace net::http1
