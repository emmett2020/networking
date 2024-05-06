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


#include "net/utils/string_compare.h"

namespace net::utils {

  bool strcasecmp(std::string_view s1, std::string_view s2) {
    std::size_t size = s1.size();
    if (size != s2.size()) {
      return false;
    }

    const auto* p1 = reinterpret_cast< const unsigned char*>(s1.data());
    const auto* p2 = reinterpret_cast< const unsigned char*>(s2.data());

    constexpr std::size_t zero = 0;
    constexpr std::size_t step = sizeof(std::size_t);
    constexpr std::size_t mask = 0xDFDFDFDFDFDFDFDFz & ~zero;

    for (; size >= step; p1 += step, p2 += step, size -= step) {
      if (((convert_to_int(p1) ^ convert_to_int(p2)) & mask) != 0) {
        return false;
      }
    }
    for (; size > 0; ++p1, ++p2, --size) {
      if (((*p1 ^ *p2) & 0xDF) != 0) {
        return false;
      }
    }
    return true;
  }

} // namespace net::utils
