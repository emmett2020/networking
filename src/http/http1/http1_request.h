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
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "http/http_common.h"
#include "http/http_concept.h"
#include "http/http_metric.h"
#include "http/http_option.h"
#include "utils/string_hash.h"

// TODO:
// 1. use std::expected as return type
// 2. replace message_parser::buffer with std::string_view
// 3. complete simple_hashtable concept
// 4. add heterogenerous access for unordered_map, especially http_headers type

namespace net::http::http1 {
  struct ensure_lower_case_t { };

  class http1_headers {
    [[nodiscard]] bool contain(std::string_view name) const noexcept {
      headers_.find();
      for (const auto& [n, v]: headers_) {
        // if (strcasecmp(n.c_str(), const char* s2)) {


        std::ranges::equal(name, n, [](unsigned char c1, unsigned char c2) { return c1 == c2; });
      }

      return headers_.contains(name);
    }

    // std::ranges::transform(name, name.begin(), tolower);

    // [[nodiscard]] bool contain(std::string name) const noexcept {
    //   std::ranges::transform(name, name.begin(), tolower);
    //   return table_.contains(name);
    // }

    [[nodiscard]] bool
      contain(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) const noexcept {

      return headers_.contains(name);
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const std::string>>
      value(std::string name) const noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      if (const auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const std::string>>
      value(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) const noexcept {
      if (const auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<std::string>> value(std::string name) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      if (auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<std::string>>
      value(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) noexcept {
      if (auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    // Return whether successfully added.
    bool insert(std::string name, const std::string& value) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      if (headers_.contains(name)) {
        return false;
      }
      headers_[std::move(name)] = value;
      return true;
    }

    void update(std::string name, const std::string& value) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      headers_[std::move(name)] = value;
    }

    void update(
      std::string name,
      const std::string& value,
      [[maybe_unused]] ensure_lower_case_t _) noexcept {
      headers_[std::move(name)] = value;
    }

    bool remove(std::string name) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      return headers_.erase(name) == 1;
    }

    bool remove(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) noexcept {
      return headers_.erase(name) == 1;
    }

    void clear() noexcept {
      headers_.clear();
    }

    void append_value(std::string name, const std::string& value) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      headers_[name] += value;
    }

    void append_value(
      const std::string& name,
      const std::string& value,
      [[maybe_unused]] ensure_lower_case_t _) noexcept {
      headers_[name] += value;
    }

    [[nodiscard]] std::size_t size() const noexcept {
      return headers_.size();
    }


   private:
    std::unordered_map<std::string, std::string, util::string_hash, std::equal_to<>> headers_;
  };

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

    const param_t& params() const noexcept {
      return params_;
    }

    param_t& params() noexcept {
      return params_;
    }

    bool contain_param(const std::string& name) const noexcept {
      return params_.contains(name);
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

    bool insert_param(std::string name, std::string value) noexcept {
      if (params_.contains(name)) {
        return false;
      }
      params_[std::move(name)] = std::move(value);
      return true;
    }

    // Return whether name exists.
    bool delete_param(const std::string& name) noexcept {
      return params_.erase(name) == 1;
    }

    void update_param(std::string name, std::string value) noexcept {
      params_[std::move(name)] = std::move(value);
    }

    void clear_params() noexcept {
      params_.clear();
    }

    void append_param_value(const std::string& name, const std::string& val) noexcept {
      params_[name] += val;
    }

    std::size_t count_param() const noexcept {
      return params_.size();
    }

    const header_t& headers() const noexcept {
      return headers_;
    }

    header_t& headers() noexcept {
      return headers_;
    }

    bool contain_header(std::string name) const noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      return headers_.contains(name);
    }

    bool contain_header(
      const std::string& name,
      [[maybe_unused]] ensure_lower_case_t _) const noexcept {
      return headers_.contains(name);
    }

    std::optional<std::reference_wrapper<const std::string>>
      header_value(std::string name) const noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      if (const auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<const std::string>>
      header_value(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) const noexcept {
      if (const auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<std::string>> header_value(std::string name) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      if (auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::reference_wrapper<std::string>>
      header_value(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) noexcept {
      if (auto it = headers_.find(name); it != headers_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    // Return whether successfully added.
    bool insert_header(std::string name, std::string value) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      if (headers_.contains(name)) {
        return false;
      }
      headers_[std::move(name)] = std::move(value);
      return true;
    }

    void update_header(std::string name, std::string value) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      headers_[std::move(name)] = std::move(value);
    }

    void update_header(
      std::string name,
      std::string value,
      [[maybe_unused]] ensure_lower_case_t _) noexcept {
      headers_[std::move(name)] = std::move(value);
    }

    bool delete_header(std::string name) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      return headers_.erase(name) == 1;
    }

    bool delete_header(const std::string& name, [[maybe_unused]] ensure_lower_case_t _) noexcept {
      return headers_.erase(name) == 1;
    }

    void clear_headers() noexcept {
      headers_.clear();
    }

    void append_header_value(std::string name, const std::string& value) noexcept {
      std::ranges::transform(name, name.begin(), tolower);
      headers_[name] += value;
    }

    void append_header_value(
      const std::string& name,
      const std::string& value,
      [[maybe_unused]] ensure_lower_case_t _) noexcept {
      headers_[name] += value;
    }

    std::size_t count_header() const noexcept {
      return headers_.size();
    }

    void update_metric(
      const Metric::timepoint_t& start,
      const Metric::timepoint_t& stop,
      std::size_t size) noexcept {
      metric_.update_time(start, stop);
      metric_.update_size(size);
    }

    http_method method = http_method::unknown;
    http_scheme scheme = http_scheme::unknown;
    http_version version = http_version::unknown;

    uint16_t port = 0;
    std::string host;
    std::string path;
    std::string uri;
    std::string body;
    std::size_t content_length = 0;

    header_t headers_;
    param_t params_;
    Metric metric_;
  };

  using http1_client_request = request<http_message_direction::receive_from_client>;
  using http1_server_request = request<http_message_direction::send_to_server>;
} // namespace net::http::http1
