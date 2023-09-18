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

#include "http/http_common.h"

namespace net::http {

struct Options {
  uint32_t max_length_of_uri_path = 4096;
  uint32_t max_length_of_uri_param_name = 1024;
  uint32_t max_length_of_uri_param_value = 1024;
  uint32_t max_size_of_uri_params = 1024;
};

// A request received from a client.
struct RequestBase {
  HttpMethod method{HttpMethod::kUnknown};
  HttpScheme scheme{HttpScheme::kUnknown};
  HttpVersion version{HttpVersion::kUnknown};
  uint16_t port = 0;
  std::string host;
  std::string path;
  std::string uri;
  std::string body;
  std::size_t content_length{0};
  std::unordered_map<std::string, std::string> headers;
  std::unordered_map<std::string, std::string> params;
};

struct Request : public RequestBase {

  HttpMethod Method() const noexcept { return method; }

  HttpScheme Scheme() const noexcept { return scheme; }

  std::string_view Host() const noexcept { return host; }

  std::uint16_t Port() const noexcept { return port; }

  std::string_view Path() const noexcept { return path; }

  std::string_view Uri() const noexcept { return uri; }

  HttpVersion Version() const noexcept { return version; }

  std::string_view Body() const noexcept { return body; }

  void SetHeaderContentLength(std::size_t length) noexcept {
    content_length = length;
  }

  std::unordered_map<std::string, std::string> Params() const noexcept {
    return params;
  }

  std::unordered_map<std::string, std::string>& Params() noexcept {
    return params;
  }

  std::optional<std::string_view> ParamValue(
      const std::string& param_key) const noexcept {
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

  std::optional<std::string_view> HeaderValue(
      const std::string& header_name) const noexcept {
    for (const auto& [name, value] : headers) {
      if (::strcasecmp(name.c_str(), header_name.c_str()) == 0) {
        return value;
      }
    }
    return std::nullopt;
  }

  bool ContainsHeader(const std::string& header_name) const noexcept {
    return std::any_of(headers.cbegin(), headers.cend(),
                       [header_name](const auto& p) noexcept -> bool {
                         return ::strcasecmp(p.first.c_str(),
                                             header_name.c_str()) == 0;
                       });
  }
};

}  // namespace net::http
