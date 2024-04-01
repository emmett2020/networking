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

#include <functional>
#include <string_view>
#include <string>
#include <ranges>

namespace net::util {
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

  struct case_insensitive_string_hash {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const {
      auto v = str | std::views::transform([](auto c) { return tolower(c); });
      return hash_type{}(str);
      // return hash_type{}(str);
    }

    std::size_t operator()(std::string_view str) const {
      auto v = str | std::views::transform([](auto c) { return tolower(c); });
      return hash_type{}(str);
    }

    std::size_t operator()(const std::string& str) const {
      return hash_type{}(str);
    }
  };
} // namespace net::util
