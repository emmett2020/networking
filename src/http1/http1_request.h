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

  // A request received from a client.
  struct RequestBase {
    http_method method{http_method::unknown};
    http_scheme scheme{http_scheme::unknown};
    http_version version{http_version::unknown};
    uint16_t port = 0;
    std::string host;
    std::string path;
    std::string uri;
    std::string body;
    std::size_t content_length{0};
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;

    uint32_t keep_alive_reuse_count{0};
    socket_metric metric;
  };

  // A request could be used while client send to server or server send to client.
  template <message_direction MessageDirection>
  struct Request : public RequestBase {
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

    http_method Method() const noexcept {
      return method;
    }

    http_scheme Scheme() const noexcept {
      return scheme;
    }

    std::string_view Host() const noexcept {
      return host;
    }

    std::uint16_t Port() const noexcept {
      return port;
    }

    std::string_view Path() const noexcept {
      return path;
    }

    std::string_view Uri() const noexcept {
      return uri;
    }

    http_version Version() const noexcept {
      return version;
    }

    std::string_view Body() const noexcept {
      return body;
    }

    void SetHeaderContentLength(std::size_t length) noexcept {
      content_length = length;
    }

    std::unordered_map<std::string, std::string> Params() const noexcept {
      return params;
    }

    std::unordered_map<std::string, std::string>& Params() noexcept {
      return params;
    }

    std::optional<std::string_view> ParamValue(const std::string& param_key) const noexcept {
      auto it = params.find(param_key);
      if (it == params.end()) {
        return std::nullopt;
      }
      return it->second;
    }

    bool ContainsParam(const std::string& param_key) const noexcept {
      return params.contains(param_key);
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

    bool ContainsHeader(std::string_view header_name) const noexcept {
      return std::any_of(
        headers.cbegin(), headers.cend(), [header_name](const auto& p) noexcept -> bool {
          return ::strcasecmp(p.first.c_str(), header_name.data()) == 0;
        });
    }

    void update_metric(const timepoint& start, const timepoint& stop, std::size_t size) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(start - stop);
      if (metric.time.first.time_since_epoch().count() == 0) {
        metric.time.first = start;
      }
      metric.time.last = stop;
      metric.time.elapsed += elapsed;
      metric.size.total += size;
    }
  };

  using client_request = Request<message_direction::receive>;
  using server_request = Request<message_direction::send>;
} // namespace net::http1
