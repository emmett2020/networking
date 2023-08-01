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

#include "http/http_request_parser.h"
#include <cctype>
#include <string_view>

namespace net::http {

inline constexpr bool IsSpecial(char input) noexcept {
  switch (input) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
      return true;
    default:
      return false;
  }
}

void RequestParser::Reset() noexcept {
  state_ = ParseState::kMethodBegin;
  buffer_.clear();
}

// Char is signed in most platforms.
// NOLINTNEXTLINE
auto RequestParser::Consume(Request& req, char input) noexcept -> ParseResult {
  switch (state_) {
    case ParseState::kMethodBegin:
      if (std::iscntrl(input) == 1 || IsSpecial(input)) {
        return ParseResult::kFail;
      } else {
        buffer_.clear();
        buffer_.push_back(input);
        state_ = ParseState::kMethod;
        return ParseResult::kNeedMore;
      }
    case ParseState::kMethod:
      if (std::isspace(input) == 1) {
        req.method = StringToHttpMethod({buffer_.data(), buffer_.size()});
        buffer_.clear();
        state_ = ParseState::kSpaceBeforeUri;
        return ParseResult::kNeedMore;
      } else if (std::iscntrl(input) == 1 || IsSpecial(input)) {
        return ParseResult::kFail;
      } else {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      }
    case ParseState::kSpaceBeforeUri:
      if (std::isspace(input) == 1) {
        return ParseResult::kNeedMore;
      } else if (input == '/') {
        buffer_.push_back(input);
        state_ = ParseState::kAfterSlashInUri;
        return ParseResult::kNeedMore;
      } else if (std::isalpha(input) == 1) {
        buffer_.push_back(input);
        state_ = ParseState::kSchema;
        return ParseResult::kNeedMore;
      }
    case ParseState::kSchema:
      if (std::isalnum(input) == 1 || input == '+' || input == '-' ||
          input == '.') {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      } else if (input == ':') {
        state_ = ParseState::kSchemaSlash;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kSchemaSlash:
      if (input == '/') {
        buffer_.push_back(input);
        state_ = ParseState::kSchemaSlashSlash;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kSchemaSlashSlash:
      if (input == '/') {
        buffer_.push_back(input);
        state_ = ParseState::kHostBegin;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHostBegin:
      if (input == '[') {
        state_ = ParseState::kHostIpLiteral;
      } else {
        state_ = ParseState::kHost;
      }
    case ParseState::kHost:
      if (std::isalnum(input) == 1 || input == '.' || input == '-') {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      } else {
        state_ = ParseState::kHostEnd;
      }
    case ParseState::kHostEnd:
      if (input == ':') {
        state_ = ParseState::kPort;
        return ParseResult::kNeedMore;
      } else if (input == '/') {
        state_ = ParseState::kAfterSlashInUri;
        return ParseResult::kNeedMore;
      } else if (input == '?') {
        state_ = ParseState::kParamName;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHostIpLiteral:
      if (std::isalnum(input) == 1 || input == ':' || input == '.') {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      } else if (input == ']') {
        state_ = ParseState::kHostEnd;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kPort:
      if (std::isdigit(input) == 1) {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      } else if (input == '/') {
        state_ = ParseState::kAfterSlashInUri;
        return ParseResult::kNeedMore;
      } else if (input == '?') {
        state_ = ParseState::kParamName;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kAfterSlashInUri:
    case ParseState::kUri:
      if (std::isspace(input) == 1) {
        buffer_.clear();
        state_ = ParseState::kHttpVersionH;
        return ParseResult::kNeedMore;
      } else if (std::iscntrl(input) == 1) {
        return ParseResult::kFail;
      } else if (input == '?') {
        state_ = ParseState::kParamName;
        return ParseResult::kNeedMore;
      } else {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      }
    case ParseState::kParamName:
      if (input == '&') {
        req.params[buffer_].clear();
        buffer_.clear();
        return ParseResult::kNeedMore;
      } else if (std::isspace(input) == 1) {
        req.params[buffer_].clear();
        state_ = ParseState::kHttpVersionH;
        return ParseResult::kNeedMore;
      } else if (input == '=') {
        req.params[buffer_].clear();
        state_ = ParseState::kParamValue;
        return ParseResult::kNeedMore;
      } else {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      }
    case ParseState::kParamValue:
      if (input == '&') {
        buffer_.clear();
        state_ = ParseState::kParamName;
        return ParseResult::kNeedMore;
      } else if (input == ' ') {
        buffer_.clear();
        state_ = ParseState::kHttpVersionH;
        return ParseResult::kNeedMore;
      } else {
        req.params[buffer_].push_back(input);
        req.params[buffer_].clear();
        return ParseResult::kNeedMore;
      }

    case ParseState::kHttpVersionH:
      buffer_.clear();
      if (input == 'H') {
        state_ = ParseState::kHttpVersionT1;
        return ParseResult::kNeedMore;
      }
      return ParseResult::kFail;
    case ParseState::kHttpVersionT1:
      if (input == 'T') {
        state_ = ParseState::kHttpVersionT2;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionT2:
      if (input == 'T') {
        state_ = ParseState::kHttpVersionP;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionP:
      if (input == 'P') {
        state_ = ParseState::kHttpVersionSlash;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionSlash:
      if (input == '/') {
        state_ = ParseState::kHttpVersionMajorBegin;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionMajorBegin:
      if (std::isdigit(input) == 1) {
        req.http_version_major = req.http_version_major * 10 + input - '0';
        state_ = ParseState::kHttpVersionMajor;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionMajor:
      if (input == '.') {
        state_ = ParseState::kHttpVersionMinorBegin;
        return ParseResult::kNeedMore;
      } else if (std::isdigit(input) == 1) {
        req.http_version_major = req.http_version_major * 10 + input - '0';
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionMinorBegin:
      if (std::isdigit(input) == 1) {
        req.http_version_minor = req.http_version_minor * 10 + input - '0';
        state_ = ParseState::kHttpVersionMinor;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHttpVersionMinor:
      if (input == '\r') {
        state_ = ParseState::kExpectingNewline1;
        return ParseResult::kNeedMore;
      } else if (std::isdigit(input) == 1) {
        req.http_version_minor = req.http_version_minor * 10 + input - '0';
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kExpectingNewline1:
      if (input == '\n') {
        state_ = ParseState::kHeaderLineBegin;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHeaderLineBegin:
      if (input == '\r') {
        state_ = ParseState::kExpectingNewline3;
        return ParseResult::kNeedMore;
      } else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
        state_ = ParseState::kHeaderLws;
        return ParseResult::kNeedMore;
      } else if (std::iscntrl(input) == 1 || IsSpecial(input)) {
        return ParseResult::kFail;
      } else {
        buffer_.push_back(input);
        state_ = ParseState::kHeaderName;
        return ParseResult::kNeedMore;
      }
    case ParseState::kHeaderLws:
      if (input == '\r') {
        state_ = ParseState::kExpectingNewline2;
        return ParseResult::kNeedMore;
      } else if (input == ' ' || input == '\t') {
        return ParseResult::kNeedMore;
      } else if (std::iscntrl(input) == 1) {
        return ParseResult::kFail;
      } else {
        state_ = ParseState::kHeaderValue;
        req.headers[buffer_].push_back(input);
        return ParseResult::kNeedMore;
      }
    case ParseState::kHeaderName:
      if (input == ':') {
        state_ = ParseState::kSpaceBeforeHeaderValue;
        return ParseResult::kNeedMore;
      } else if (std::iscntrl(input) == 1 || IsSpecial(input)) {
        return ParseResult::kFail;
      } else {
        buffer_.push_back(input);
        return ParseResult::kNeedMore;
      }
    case ParseState::kSpaceBeforeHeaderValue:
      if (input == ' ') {
        state_ = ParseState::kHeaderValue;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kHeaderValue:
      if (input == '\r') {
        buffer_.clear();
        state_ = ParseState::kExpectingNewline2;
        return ParseResult::kNeedMore;
      } else if (std::iscntrl(input) == 1) {
        return ParseResult::kFail;
      } else {
        req.headers[buffer_].push_back(input);
        return ParseResult::kNeedMore;
      }
    case ParseState::kExpectingNewline2:
      if (input == '\n') {
        state_ = ParseState::kHeaderLineBegin;
        return ParseResult::kNeedMore;
      } else {
        return ParseResult::kFail;
      }
    case ParseState::kExpectingNewline3:
      return (input == '\n') ? ParseResult::kSuccess : ParseResult::kFail;
    default:
      return ParseResult::kFail;
  }
}  // NOLINT

}  // namespace net::http
