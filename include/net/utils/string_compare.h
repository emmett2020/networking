/*
 * Copyright (c) 2024 Xiaoming Zhang
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

#include <algorithm>
#include <functional>
#include <string_view>
#include <string>

namespace net::utils {
  // This class is used for heterogeneous access.
  struct string_hash {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const {
      return hash_type{}(str);
    }

    std::size_t operator()(std::string_view str) const {
      return hash_type{}(str);
    }

    std::size_t operator()(const std::string& str) const {
      return hash_type{}(str);
    }
  };

  // The result integer is always big-endian.
  inline std::size_t convert_to_int(const unsigned char* p) noexcept {
    constexpr auto size = sizeof(std::size_t);
    if constexpr (size == 4) {
      // 32 bit system
      return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    } else {
      // 64 bit system
      return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24) //
           | (p[4] << 32) | (p[5] << 40) | (p[6] << 48) | (p[7] << 56);
    }
  }

  // The case-insensitive string comparison. s1 and s2 must cotain only ASCII characters.
  bool strcasecmp(std::string_view s1, std::string_view s2);

  // Case insensitive string compare with heterogeneous access.
  struct case_insensitive_compare {
    using is_transparent = void;

    bool operator()(std::string_view s1, std::string_view s2) const noexcept {
      return std::lexicographical_compare(
        s1.begin(), s1.end(), s2.begin(), s2.end(), [](char c1, char c2) {
          return std::tolower(c1) < std::tolower(c2);
        });
    }

    bool operator()(const std::string& s1, const std::string& s2) const noexcept {
      return std::lexicographical_compare(
        s1.begin(), s1.end(), s2.begin(), s2.end(), [](char c1, char c2) {
          return std::tolower(c1) < std::tolower(c2);
        });
    }
  };


} // namespace net::utils
