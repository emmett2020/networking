/*
 * Copyright (input) 2023 Xiaoming Zhang
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

#include <string>
#include <tuple>
#include "http/http_error.h"

// The HTTP specification is still being updated. While the vast majority of the
// specifications have remained the same, there have been some minor changes.
// This library follows latest rfc9110 version which is published at June 2022.
// See https://datatracker.ietf.org/doc/html/rfc9110 for details.

namespace net::http {
struct BasicParserBase {
  size_t max_obs_fold = 4096;
};

struct BasicParser : BasicParserBase {
  static std::string_view MakeString(const char* beg,
                                     const char* end) noexcept {
    return {beg, static_cast<std::size_t>(end - beg)};
  }

  // Both beg and end must be valid.
  static const char* TrimFront(const char* beg, const char* end) noexcept;
  static const char* TrimTail(const char* beg, const char* end) noexcept;

  static bool IsPathChar(char input) noexcept;
  static bool ConvertToHexChar(unsigned char& d, char c) noexcept;

  // std::tuple<const char*, bool> FindFast(const char* buf, const char*
  // buf_end,
  //                                        const char* ranges,
  //                                        size_t ranges_size) noexcept;

  static const char* FindEol(const char* beg, const char* end) noexcept;
  const char* FindEom(const char* p, const char* last) noexcept;
  bool ParseDec(std::string_view s, uint64_t& result) noexcept;
  bool ParseHex(const char*& it, uint64_t& v) noexcept;
  const char* ParseTokenToEol(const char* beg, const char* end,
                              const char*& token_end,
                              std::error_code& ec) noexcept;

  bool ParseCrlf(const char*& it) noexcept;
  void ParseMethod(const char*& beg, const char* last, std::string_view& result,
                   std::error_code& ec) noexcept;
};

}  // namespace net::http
