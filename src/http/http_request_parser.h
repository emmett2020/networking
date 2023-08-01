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
#include "http/http_common.h"
#include "http/http_request.h"

namespace net::http {

class RequestParser {
 public:
  enum class ParseResult { kSuccess, kFail, kNeedMore };

  RequestParser() noexcept { buffer_.reserve(kBufferSize); }

  void Reset() noexcept;

  template <typename InputIterator>
  std::tuple<ParseResult, InputIterator> Parse(Request& req,
                                               InputIterator begin,
                                               InputIterator end) {
    while (begin != end) {
      ParseResult result = Consume(req, *begin++);
      if (result == ParseResult::kSuccess || result == ParseResult::kFail) {
        return std::make_tuple(result, begin);
      }
    }
    return std::make_tuple(ParseResult::kNeedMore, begin);
  }

 private:
  enum class ParseState {
    kMethodBegin,
    kMethod,
    kSpaceBeforeUri,
    kUri,
    kUriBegin,
    kAfterSlashInUri,
    kSchema,
    kSchemaSlash,
    kSchemaSlashSlash,
    kHostBegin,
    kHost,
    kHostEnd,
    kHostIpLiteral,
    kPort,
    kParamName,
    kParamValue,
    kSpaceBeforeHttpVersion,
    kHttpVersionH,
    kHttpVersionT1,
    kHttpVersionT2,
    kHttpVersionP,
    kHttpVersionSlash,
    kHttpVersionMajorBegin,
    kHttpVersionMajor,
    kHttpVersionMinorBegin,
    kHttpVersionMinor,
    kExpectingNewline1,
    kHeaderLineBegin,
    kHeaderLws,
    kHeaderName,
    kSpaceBeforeHeaderValue,
    kHeaderValue,
    kExpectingNewline2,
    kExpectingNewline3,
  };

  ParseResult Consume(Request& req, char input) noexcept;

  static constexpr size_t kBufferSize = 1024;
  ParseState state_{};
  std::string buffer_{};
};

}  // namespace net::http
