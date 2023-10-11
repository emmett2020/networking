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

#include <fmt/core.h>
#include <strings.h>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <limits>
#include <span>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>

#include "http1/http_common.h"
#include "http1/http_concept.h"
#include "http1/http_error.h"
#include "http1/http_request.h"
#include "http1/http_response.h"

// TODO(xiaoming): Refactor all documents. Remove the text we copied.
// TODO(xiaoming): Still need to implement and optimize this parser. Write more
// test case to make this parser robust. This is a long time work, we need to
// implement others first.

namespace net::http1 {
  namespace detail {

    /*
     0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel
     8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si
    16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb
    24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us
    32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '
    40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /
    48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7
    56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?
    64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G
    72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O
    80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W
    88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _
    96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g
   104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o
   112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w
   120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del
*/
    constexpr std::array<uint8_t, 256> kTokenTable = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 16
      0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, // 32
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 48
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 64
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, // 80
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 96
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, // 112
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 128
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 144
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 160
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 176
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 192
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 208
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 224
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 240-255
    };

    /**
 * @brief   Check whether a character meets the requirement of HTTP tokens.
 * @details Tokens are short textual identifiers that do not include whitespace
 *          or delimiters.
 *          token          = 1*tchar
 *          tchar          = "!" | "#"   | "$"   | "%" | "&" | "'" | "*"
 *                           "+" | "-"   | "."   | "^" | "_" | "`" | "|"
 *                           "~" | DIGIT | ALPHA
 * @param input The charater to be checked.
 */
    inline bool IsToken(uint8_t input) noexcept {
      return static_cast<bool>(kTokenTable[input]);
    }

    /**
 * @brief   Check whether a character meets the requirement of HTTP URI.
 * @details Tokens are short textual identifiers that do not include whitespace
 *          or delimiters.
 *          token          = 1*tchar
 *          tchar          = "!" | "#"   | "$"   | "%" | "&" | "'" | "*"
 *                           "+" | "-"   | "."   | "^" | "_" | "`" | "|"
 *                           "~" | DIGIT | ALPHA
 * @param Input the charater to be checked.
 */
    inline bool IsPathChar(uint8_t input) noexcept {
      constexpr std::array<uint8_t, 256> kTable{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //   0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  16
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  32
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  48
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  64
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  80
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  96
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // 112
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 128
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 144
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 160
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 176
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 192
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 208
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 224
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // 240
      };
      return static_cast<bool>(kTable[input]);
    }

    /**
 * @brief A helper function to make string view use two pointers.
 * @param first The start address of the data.
 * @param last  The end address of the data. The data range is half-opened.
 * @attention To be consistent with ParseXXX functions, the data range of this
 *            function is also half-open: [first, last).
 */
    constexpr std::string_view ToString(const char* first, const char* last) noexcept {
      return {first, static_cast<std::size_t>(last - first)};
    }

    /**
 * @brief Skip the whitespace from front.
 * @param first The start address of the data.
 * @param last  The end address of the data. The data range is half-opened.
 * @return The pointer which points to the first non-whitespace position or last
 *         which represents all charater between [first, last) is white space.
 */
    constexpr const char* TrimFront(const char* first, const char* last) noexcept {
      while (first < last && (*first == ' ' || *first == '\t')) {
        ++first;
      }
      return first;
    }

    // Helper function to compare 3 characters.
    inline bool Compare3Char(const char* p, char c0, char c1, char c2) noexcept {
      return p[0] == c0 && p[1] == c1 && p[2] == c2;
    }

    // Helper function to compare 4 characters.
    inline bool Compare4Char(const char* p, char c0, char c1, char c2, char c3) noexcept {
      return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3;
    }

    // Helper function to compare 5 characters.
    inline bool Compare5Char(const char* p, char c0, char c1, char c2, char c3, char c4) noexcept {
      return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4;
    }

    // Helper function to compare 6 characters.
    inline bool
      Compare6Char(const char* p, char c0, char c1, char c2, char c3, char c4, char c5) noexcept {
      return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4 && p[5] == c5;
    }

    // Helper function to compare 7 characters.
    inline bool Compare7Char(
      const char* p,
      char c0,
      char c1,
      char c2,
      char c3,
      char c4,
      char c5,
      char c6) noexcept {
      return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4 && p[5] == c5
          && p[6] == c6;
    }

    // Helper function to compare 8 characters.
    inline bool Compare8Char(
      const char* p,
      char c0,
      char c1,
      char c2,
      char c3,
      char c4,
      char c5,
      char c6,
      char c7) noexcept {
      return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4 && p[5] == c5
          && p[6] == c6 && p[7] == c7;
    }

    // Helper function to compare "http" characters in case-sensitive mode.
    inline bool CaseCompareHttpChar(const char* p) {
      return (p[0] | 0x20) == 'h' && (p[1] | 0x20) == 't' && (p[2] | 0x20) == 't'
          && (p[3] | 0x20) == 'p';
    }

    // Helper function to compare "https" characters in case-sensitive mode.
    inline bool CaseCompareHttpsChar(const char* p) {
      return (p[0] | 0x20) == 'h' && (p[1] | 0x20) == 't' && (p[2] | 0x20) == 't'
          && (p[3] | 0x20) == 'p' && (p[4] | 0x20) == 's';
    }

    inline HttpMethod ToHttpMethod(const char* const beg, const char* const end) {
      const size_t len = end - beg;
      if (len == 3) {
        if (Compare3Char(beg, 'G', 'E', 'T')) {
          return HttpMethod::kGet;
        }
        if (Compare3Char(beg, 'P', 'U', 'T')) {
          return HttpMethod::kPut;
        }
      } else if (len == 4) {
        if (Compare4Char(beg, 'P', 'O', 'S', 'T')) {
          return HttpMethod::kPost;
        }
        if (Compare4Char(beg, 'H', 'E', 'A', 'D')) {
          return HttpMethod::kHead;
        }
      } else if (len == 5) {
        if (Compare5Char(beg, 'T', 'R', 'A', 'C', 'E')) {
          return HttpMethod::kTrace;
        }
        if (Compare5Char(beg, 'P', 'U', 'R', 'G', 'E')) {
          return HttpMethod::kPurge;
        }
      } else if (len == 6) {
        if (Compare6Char(beg, 'D', 'E', 'L', 'E', 'T', 'E')) {
          return HttpMethod::kDelete;
        }
      } else if (len == 7) {
        if (Compare7Char(beg, 'O', 'P', 'T', 'I', 'O', 'N', 'S')) {
          return HttpMethod::kOptions;
        }

        if (Compare7Char(beg, 'C', 'O', 'N', 'T', 'R', 'O', 'L')) {
          return HttpMethod::kControl;
        }
        if (Compare7Char(beg, 'C', 'O', 'N', 'N', 'E', 'C', 'T')) {
          return HttpMethod::kConnect;
        }
      }
      return HttpMethod::kUnknown;
    }

    constexpr std::uint16_t GetDefaultPortByHttpScheme(HttpScheme scheme) noexcept {
      if (scheme == HttpScheme::kHttp) {
        return 80;
      }
      if (scheme == HttpScheme::kHttps) {
        return 443;
      }
      return 0;
    }

  } // namespace detail

  using std::error_code;

  /*
 * Introduction:
 *   This parser is used for parse HTTP/1.0 and HTTP/1.1 message. According to
 * RFC9112, An HTTP/1.1 message consists of a start-line followed by a CRLF and
 * a sequence of octets in a format similar to the Internet Message Format :
 * zero or more header field lines (collectively referred to as the "headers" or
 * the "header section"), an empty line indicating the end of the header
 * section, and an optional message body.
 *          HTTP-message = start-line CRLF
 *                         *( field-line CRLF )
 *                         CRLF
 *                         [ message-body ]
 * Message:
 *   A message can be either a request from client to server or a response from
 * server to client. Syntactically, the two types of messages differ only in the
 * start-line, which is either a request-line (for requests) or a status-line
 * (for responses), and in the algorithm for determining the length of the
 * message body.
 *
 * Uri:
 *   A URI consists of many parts. In general, a URI can
 * contain at most scheme, host, port, path, query parameters, and fragment.
 * Each subpart of the URI is described in detail in the corresponding parsing
 * function. For URI parse, in addition to the RFC, I also relied heavily on
 * whatwg standards. The relevant reference links are given below.
 *
 * Code guide:
 *   This Parser uses the ParseXXX function to parse http messages. Protocol
 * message parsing is always error-prone, and this parser tries its best to
 * ensure that the code style of the parsing function is uniform.
 *   1. All ParserXXX functions take three parameters. The first argument is the
 * start of the buffer that this particular ParseXXX function needs to parse,
 * the second argument is the end of the buffer, and the third argument is error
 * code, which contains the specific error message. Note that the first argument
 * is the reference type, which is assigned and returned by the particular
 * ParseXXX function.
 *   2.
 *
 * The correctness of the parsing is ensured by a large
 * number of associated unit tests.
 *
 * @see RFC 9110: https://datatracker.ietf.org/doc/html/rfc9110
 * @see RFC 9112: https://datatracker.ietf.org/doc/html/rfc9112
 * @see whatwg URL specification: https://url.spec.whatwg.org/#urls
 */

  template <http_message Message>
  class MessageParser {
   public:
    enum class MessageState {
      kNothingYet,
      kStartLine,
      kHeader,
      kExpectingNewline,
      kBody,
      kCompleted
    };

    explicit MessageParser(Message* message) noexcept
      : message_(message) {
      name_.reserve(8192);
    }

    void Reset(Message* message) noexcept {
      message_ = message;
      Reset();
    }

    void Reset() noexcept {
      message_state_ = MessageState::kNothingYet;
      header_state_ = HeaderState::kName;
      uri_state_ = UriState::kInitial;
      param_state_ = ParamState::kName;
      inner_parsed_len_ = 0;
      name_.clear();
    }

    [[nodiscard]] Message* AssociatedMessage() const noexcept {
      return message_;
    }

    // TODO(xiaoming): we need span<byte>
    std::size_t Parse(std::span<char> buffer, error_code& ec) {
      assert(!ec &&
           "net::http1::MessageParser::parse() failed. The parameter ec should "
           "be clear.");
      Argument arg{
        .buffer_beg = buffer.data(),
        .buffer_end = buffer.data() + buffer.size(),
        .parsed_len = 0,
      };
      while (!ec) {
        switch (message_state_) {
        case MessageState::kNothingYet:
        case MessageState::kStartLine: {
          if constexpr (http_request<Message>) {
            ParseRequestLine(arg, ec);
          } else {
            ParseStatusLine(arg, ec);
          }
          break;
        }
        case MessageState::kExpectingNewline: {
          ParseExpectingNewLine(arg, ec);
          break;
        }
        case MessageState::kHeader: {
          ParseHeader(arg, ec);
          break;
        }
        case MessageState::kBody: {
          ParseBody(arg, ec);
          break;
        }
        case MessageState::kCompleted: {
          return arg.parsed_len;
        }
        }
      }
      // If fatal error occurs, return zero as parsed size.
      return ec == Error::kNeedMore ? arg.parsed_len : 0;
    }

    [[nodiscard]] MessageState State() const noexcept {
      return message_state_;
    }

   private:
    struct Argument {
      const char* buffer_beg{nullptr};
      const char* buffer_end{nullptr};
      std::size_t parsed_len{0};
    };

    enum class RequestLineState {
      kMethod,
      kSpacesBeforeUri,
      kUri,
      kSpacesBeforeHttpVersion,
      kVersion,
    };

    enum class StatusLineState {
      kVersion,
      kStatusCode,
      kReason,
    };

    enum class UriState {
      kInitial,
      kScheme,
      kHost,
      kPort,
      kPath,
      kParams,
      kCompleted
    };

    enum class HeaderState {
      kName,
      kSpacesBeforeValue,
      kValue,
      kHeaderLineEnding
    };

    enum class ParamState {
      kName,
      kValue,
      kCompleted
    };

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9112:
   *               request-line   = method SP request-target SP HTTP-version
   *   A request-line begins with a method token, followed by a single space
   * (SP), the request-target, and another single space (SP), and ends with the
   * protocol version.
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-request-line
   */
    void ParseRequestLine(Argument& arg, error_code& ec) {
      while (!ec) {
        switch (request_line_state_) {
        case RequestLineState::kMethod: {
          ParseMethod(arg, ec);
          break;
        }
        case RequestLineState::kSpacesBeforeUri: {
          ParseSpacesBeforeUri(arg, ec);
          break;
        }
        case RequestLineState::kUri: {
          ParseUri(arg, ec);
          break;
        }
        case RequestLineState::kSpacesBeforeHttpVersion: {
          ParseSpacesBeforeVersion(arg, ec);
          break;
        }
        case RequestLineState::kVersion: {
          ParseRequestHttpVersion(arg, ec);
          return;
        }
        }
      }
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9112:
   *                        method = token
   * The method token is case-sensitive. By convention, standardized methods are
   * defined in all-uppercase US-ASCII letters and not allowd to be empty. If no
   * error occurred, the method field of inner request will be filled and the
   * state_ of this parser will be changed from kMethod to kSpacesBeforeUri.
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-method
   */
    void ParseMethod(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* method_beg = arg.buffer_beg + arg.parsed_len;
      for (const char* p = method_beg; p < arg.buffer_end; ++p) {
        // Collect method string until met first non-method charater.
        if (detail::IsToken(*p)) [[likely]] {
          continue;
        }

        // The first character after method string must be whitespace.
        if (*p != ' ') {
          ec = Error::kBadMethod;
          return;
        }

        // Empty method is not allowed.
        if (p == method_beg) {
          ec = Error::kEmptyMethod;
          return;
        }

        message_->method = detail::ToHttpMethod(method_beg, p);
        if (message_->method == HttpMethod::kUnknown) {
          ec = Error::kBadMethod;
          return;
        }

        arg.parsed_len += p - method_beg;
        request_line_state_ = RequestLineState::kSpacesBeforeUri;
        return;
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details Parse white spaces before http URI. Multiply whitespaces are
   * allowd between method and URI.
   */
    void ParseSpacesBeforeUri(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* spaces_beg = arg.buffer_beg + arg.parsed_len;
      const char* p = detail::TrimFront(spaces_beg, arg.buffer_end);
      if (p == arg.buffer_end) [[unlikely]] {
        ec = Error::kNeedMore;
        return;
      }
      arg.parsed_len += p - spaces_beg;
      request_line_state_ = RequestLineState::kUri;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details The "http" and "https" URI defined as below:
   *                     http-URI  = "http"  "://" authority path [ "?" query ]
   *                     https-URI = "https" "://" authority path [ "?" query ]
   *                     authority = host ":" port
   * Parse http request uri. Use the first non-whitespace charater to judge the
   * URI form. That's to say, if the URI starts with '/', the URI form is
   * absolute-path. Otherwise it's absolute-form. You may need to read RFC9112
   * to get a detailed difference with these two forms.
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-request-target
   */
    void ParseUri(Argument& arg, error_code& ec) noexcept {
      static_assert(http_request<Message>);
      const char* uri_beg = arg.buffer_beg + arg.parsed_len;
      if (uri_beg >= arg.buffer_end) {
        ec = Error::kNeedMore;
        return;
      }

      uri_state_ = UriState::kInitial;
      while (!ec) {
        switch (uri_state_) {
        case UriState::kInitial: {
          if (*uri_beg == '/') {
            message_->port = 80;
            uri_state_ = UriState::kPath;
          } else {
            uri_state_ = UriState::kScheme;
          }
          inner_parsed_len_ = 0;
          break;
        }
        case UriState::kScheme: {
          ParseScheme(arg, ec);
          break;
        }
        case UriState::kHost: {
          ParseHost(arg, ec);
          break;
        }
        case UriState::kPort: {
          ParsePort(arg, ec);
          break;
        }
        case UriState::kPath: {
          ParsePath(arg, ec);
          break;
        }
        case UriState::kParams: {
          ParseParams(arg, ec);
          break;
        }
        case UriState::kCompleted: {
          request_line_state_ = RequestLineState::kSpacesBeforeHttpVersion;
          message_->uri = {uri_beg, uri_beg + inner_parsed_len_};
          arg.parsed_len += inner_parsed_len_;
          inner_parsed_len_ = 0;
          return;
        }
        }
      }
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                        scheme = "+" | "-" | "." | DIGIT | ALPHA
   * Parse http request scheme. The scheme are case-insensitive and normally
   * provided in lowercase. This library mainly parse two schemes: "http"
   * URI scheme and "https" URI scheme. If no error occurs, the inner request's
   * scheme field will be filled.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
   */
    void ParseScheme(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* uri_beg = arg.buffer_beg + arg.parsed_len;
      const char* scheme_beg = uri_beg + inner_parsed_len_;
      for (const char* p = scheme_beg; p < arg.buffer_end; ++p) {
        // Skip the scheme character to find first colon.
        if (
          ((*p | 0x20) >= 'a' && (*p | 0x20) <= 'z') || //
          (*p >= '0' && *p <= '9') ||                   //
          (*p == '+' || *p == '-' || *p == '.')) {
          continue;
        }

        // Need more data.
        if (arg.buffer_end - p < 2) {
          ec = Error::kNeedMore;
          return;
        }

        // The characters next to the first non-scheme character must be "://".
        if (*p != ':' || *(p + 1) != '/' || *(p + 2) != '/') {
          ec = Error::kBadScheme;
          return;
        }

        uint32_t scheme_len = p - scheme_beg;
        if (scheme_len >= 5 && detail::CaseCompareHttpChar(scheme_beg)) {
          message_->scheme = HttpScheme::kHttps;
        } else if (scheme_len >= 4 && detail::CaseCompareHttpChar(scheme_beg)) {
          message_->scheme = HttpScheme::kHttp;
        } else {
          message_->scheme = HttpScheme::kUnknown;
        }

        inner_parsed_len_ = scheme_len + 3;
        uri_state_ = UriState::kHost;
        return;
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                          host = "-" | "." | DIGIT | ALPHA
   * Parse http request host identifier. The host can be IP address style ASCII
   * string (both IPv4 address and IPv6 address) or a domain name ASCII string
   * which could be DNS resolved future. Note that empty host identifier is not
   * allowed in a "http" or "https" URI scheme. If no error occurs, the inner
   * request's host field will be filled.
   * @see
   * https://datatracker.ietf.org/doc/html/rfc9110#name-authoritative-access
   * https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
   */
    void ParseHost(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* uri_beg = arg.buffer_beg + arg.parsed_len;
      const char* host_beg = uri_beg + inner_parsed_len_;
      for (const char* p = host_beg; p < arg.buffer_end; ++p) {
        // Collect host string until met first non-host charater.
        if (
          ((*p | 0x20) >= 'a' && (*p | 0x20) <= 'z') || //
          (*p >= '0' && *p <= '9') ||                   //
          (*p == '-' || *p == '.')) {
          continue;
        }

        // The charater after host must be follows.
        if (*p != ':' && *p != '/' && *p != '?' && *p != ' ') {
          ec = Error::kBadHost;
          return;
        }

        // A empty host is not allowed at "http" scheme and "https" scheme.
        if (
          p == host_beg
          && (message_->scheme == HttpScheme::kHttp || message_->scheme == HttpScheme::kHttps)) {
          ec = Error::kBadHost;
          return;
        }

        // Fill host field of inner request.
        uint32_t host_len = p - host_beg;
        message_->host = {host_beg, p};

        // Go to next state depends on the first charater after host identifier.
        switch (*p) {
        case ':':
          inner_parsed_len_ += host_len + 1;
          uri_state_ = UriState::kPort;
          return;
        case '/':
          message_->port = detail::GetDefaultPortByHttpScheme(message_->scheme);
          inner_parsed_len_ += host_len;
          uri_state_ = UriState::kPath;
          return;
        case '?':
          message_->port = detail::GetDefaultPortByHttpScheme(message_->scheme);
          inner_parsed_len_ += host_len + 1;
          uri_state_ = UriState::kParams;
          return;
        case ' ':
          message_->port = detail::GetDefaultPortByHttpScheme(message_->scheme);
          inner_parsed_len_ += host_len;
          uri_state_ = UriState::kCompleted;
          return;
        }
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                          port = DIGIT
   * Parse http request port. This function is used to parse http request port.
   * Http request port is a digit charater collection which used for TCP
   * connection. So the port value is less than 65535 in most Unix like
   * platforms. And according to RFC 9110, port can have leading zeros. If port
   * is elided from the URI, the default port for that scheme is used. If no
   * error occurs, the inner request's port field will be filled.
   * @see
   * https://datatracker.ietf.org/doc/html/rfc9110#name-authoritative-access
   */
    void ParsePort(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      uint16_t acc = 0;
      uint16_t cur = 0;

      const char* uri_beg = arg.buffer_beg + arg.parsed_len;
      const char* port_beg = uri_beg + inner_parsed_len_;

      for (const char* p = port_beg; p < arg.buffer_end; ++p) {
        // Calculate the port value and save it in acc variable.
        if (*p >= '0' && *p <= '9') [[likely]] {
          cur = *p - '0';
          if (acc * 10 + cur > std::numeric_limits<uint16_t>::max()) {
            ec = Error::kBadPort;
            return;
          }
          acc = acc * 10 + cur;
          continue;
        }

        // The first character next to port string should be one of follows.
        if (*p != '/' && *p != '?' && *p != ' ') {
          ec = Error::kBadPort;
          return;
        }

        // Port is zero, so use default port value based on scheme.
        if (acc == 0) {
          acc = detail::GetDefaultPortByHttpScheme(message_->scheme);
        }
        message_->port = acc;
        uint32_t port_string_len = p - port_beg;
        switch (*p) {
        case '/': {
          inner_parsed_len_ += port_string_len;
          uri_state_ = UriState::kPath;
          return;
        }
        case '?': {
          inner_parsed_len_ += port_string_len + 1;
          uri_state_ = UriState::kParams;
          return;
        }
        case ' ': {
          inner_parsed_len_ += port_string_len;
          uri_state_ = UriState::kCompleted;
          return;
        }
        } // switch
      }   // for
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                          path = token
   * Parse http request path. Http request path is defined as the path and
   * filename in a request. It does not include scheme, host, port or query
   * string. If no error occurs, the inner request's path field will be filled.
   */
    void ParsePath(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* const uri_beg = arg.buffer_beg + arg.parsed_len;
      const char* path_beg = uri_beg + inner_parsed_len_;
      for (const char* p = path_beg; p < arg.buffer_end; ++p) {
        if (*p == '?') {
          inner_parsed_len_ += p - path_beg + 1;
          message_->path = {path_beg, p};
          uri_state_ = UriState::kParams;
          return;
        }
        if (*p == ' ') {
          inner_parsed_len_ += p - path_beg;
          message_->path = {path_beg, p};
          uri_state_ = UriState::kCompleted;
          return;
        }
        if (!detail::IsPathChar(*p)) {
          ec = Error::kBadPath;
          return;
        }
      } // for
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *               parameter-name  = token
   * The parameter name is allowed to be empty only when explicitly set the
   * equal mark. In all other situations, an empty parameter name is
   * forbiddened. Note that there isn't a clear specification illustrate what to
   * do when there is a repeated key, we just use the last-occur policy now.
   */
    void ParseParamName(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* uri_beg = arg.buffer_beg + arg.parsed_len;
      const char* name_beg = uri_beg + inner_parsed_len_;
      for (const char* p = name_beg; p < arg.buffer_end; ++p) {
        if (*p == '&') {
          // Skip the "?&" or continuous '&' , which represents empty
          // parameter name and parameter value.
          if (p == name_beg || p - name_beg == 1) {
            inner_parsed_len_ += 1;
            return;
          }

          // When all characters in two ampersand don't contain =, all
          // characters in ampersand (except ampersand) are considered
          // parameter names.
          name_ = {name_beg, p};
          message_->params[name_] = "";
          inner_parsed_len_ += p - name_beg + 1;
          return;
        }

        if (*p == '=') {
          name_ = {name_beg, p};
          inner_parsed_len_ += p - name_beg + 1;
          param_state_ = ParamState::kValue;
          return;
        }

        if (*p == ' ') {
          // When there are at least one characters in '&' and ' ', trated
          // them as parameter name.
          if (name_beg != p) {
            name_ = {name_beg, p};
            message_->params[name_] = "";
          }
          inner_parsed_len_ += p - name_beg;
          param_state_ = ParamState::kCompleted;
          return;
        }

        if (!detail::IsPathChar(*p)) {
          ec = Error::kBadParams;
          return;
        }
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *               parameter-value = ( token / quoted-string )
   *               OWS             = *( SP / HTAB )
   *                               ; optional whitespace
   */
    void ParseParamValue(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* uri_beg = arg.buffer_beg + arg.parsed_len;
      const char* value_beg = uri_beg + inner_parsed_len_;
      for (const char* p = value_beg; p < arg.buffer_end; ++p) {
        if (*p == '&') {
          message_->params[name_] = {value_beg, p};
          inner_parsed_len_ += p - value_beg + 1;
          param_state_ = ParamState::kName;
          return;
        }
        if (*p == ' ') {
          message_->params[name_] = {value_beg, p};
          inner_parsed_len_ += p - value_beg;
          param_state_ = ParamState::kCompleted;
          return;
        }
        if (!detail::IsPathChar(*p)) {
          ec = Error::kBadParams;
          return;
        }
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *               parameters      = *( OWS ";" OWS [ parameter ] )
   *               parameter       = parameter-name "=" parameter-value
   * @brief Parse http request parameters.
   * @see parameters:
   *           https://datatracker.ietf.org/doc/html/rfc9110#name-parameters
   * @see quoted-string:
   *           https://datatracker.ietf.org/doc/html/rfc9110#name-quoted-strings
   * @see parse query parameter
   *           https://url.spec.whatwg.org/#query-state
   */
    void ParseParams(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      param_state_ = ParamState::kName;
      while (!ec) {
        switch (param_state_) {
        case ParamState::kName: {
          ParseParamName(arg, ec);
          break;
        }
        case ParamState::kValue: {
          ParseParamValue(arg, ec);
          break;
        }
        case ParamState::kCompleted: {
          name_.clear();
          uri_state_ = UriState::kCompleted;
          return;
        }
        } // switch
      }   // while
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details Parse white spaces before http version. Multiply whitespaces are
   * allowd between uri and http version.
   */
    void ParseSpacesBeforeVersion(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* spaces_beg = arg.buffer_beg + arg.parsed_len;
      const char* p = detail::TrimFront(spaces_beg, arg.buffer_end);
      if (p == arg.buffer_end) [[unlikely]] {
        ec = Error::kNeedMore;
        return;
      }
      arg.parsed_len += p - spaces_beg;
      request_line_state_ = RequestLineState::kVersion;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9112:
   *                 HTTP-version  = "HTTP" "/" DIGIT "." DIGIT
   * Parse http request version. HTTP's version number consists of two decimal
   * digits separated by a "." (period or decimal point). The first digit (major
   * version) indicates the messaging syntax, whereas the second digit (minor
   * version) indicates the highest minor version within that major version to
   * which the sender is conformant (able to understand for future
   * communication). Addition to http version itself, we also need "\r\n" to
   * parse together.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-protocol-version
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-http-version
   */
    void ParseRequestHttpVersion(Argument& arg, error_code& ec) {
      static_assert(http_request<Message>);
      const char* version_beg = arg.buffer_beg + arg.parsed_len;
      if (arg.buffer_end - version_beg < 10) {
        ec = Error::kNeedMore;
        return;
      }
      if (
        version_beg[0] != 'H' ||             //
        version_beg[1] != 'T' ||             //
        version_beg[2] != 'T' ||             //
        version_beg[3] != 'P' ||             //
        version_beg[4] != '/' ||             //
        std::isdigit(version_beg[5]) == 0 || //
        version_beg[6] != '.' ||             //
        std::isdigit(version_beg[7]) == 0 || //
        version_beg[8] != '\r' ||            //
        version_beg[9] != '\n') {
        ec = Error::kBadVersion;
        return;
      }

      message_->version = ToHttpVersion(version_beg[5] - '0', version_beg[7] - '0');
      arg.parsed_len += 10;
      message_state_ = MessageState::kExpectingNewline;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9112:
   *                   status-line = version SP status-code SP reason-phrase
   * Parse http status line. The first line of a response message is
   * the status-line, consisting of the protocol version, a space (SP), the
   * status code, and another space and ending with an OPTIONAL textual phrase
   * describing the status code.
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-status-line
   */
    void ParseStatusLine(Argument& arg, error_code& ec) {
      while (!ec) {
        switch (status_line_state_) {
        case StatusLineState::kVersion: {
          ParseRsponseHttpVersion(arg, ec);
          break;
        }
        case StatusLineState::kStatusCode: {
          ParseStatusCode(arg, ec);
          break;
        }
        case StatusLineState::kReason: {
          ParseReason(arg, ec);
          return;
        }
        }
      }
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details Like @ref ParseRequestHttpVersion, but doesn't need CRLF. Instead,
   * this function needs SP to indicate version is received completed.
   */
    void ParseRsponseHttpVersion(Argument& arg, error_code& ec) {
      static_assert(http_response<Message>);
      const char* version_beg = arg.buffer_beg + arg.parsed_len;
      // HTTP-name + SP
      if (arg.buffer_end - version_beg < 9) {
        ec = Error::kNeedMore;
        return;
      }
      if (
        version_beg[0] != 'H' ||             //
        version_beg[1] != 'T' ||             //
        version_beg[2] != 'T' ||             //
        version_beg[3] != 'P' ||             //
        version_beg[4] != '/' ||             //
        std::isdigit(version_beg[5]) == 0 || //
        version_beg[6] != '.' ||             //
        std::isdigit(version_beg[7]) == 0 || //
        version_beg[8] != ' ') {
        ec = Error::kBadVersion;
        return;
      }

      message_->version = ToHttpVersion(version_beg[5] - '0', version_beg[7] - '0');
      arg.parsed_len += 9;
      status_line_state_ = StatusLineState::kStatusCode;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9112:
   *                   status-code = 3DIGIT
   * The status-code element is a 3-digit integer code describing the result of
   * the server's attempt to understand and satisfy the client's corresponding
   * request.
   */
    void ParseStatusCode(Argument& arg, error_code& ec) {
      static_assert(http_response<Message>);
      const char* status_code_beg = arg.buffer_beg + arg.parsed_len;
      // 3DIGIT + SP
      if (arg.buffer_end - status_code_beg < 3 + 1) {
        ec = Error::kNeedMore;
        return;
      }
      if (status_code_beg[3] != ' ') {
        ec = Error::kBadStatus;
        return;
      }
      message_->status_code = ToHttpStatusCode({status_code_beg, 3});
      if (message_->status_code == HttpStatusCode::kUnknown) {
        ec = Error::kBadStatus;
        return;
      }
      arg.parsed_len += 4;
      status_line_state_ = StatusLineState::kReason;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9112:
   *               reason-phrase  = 1*( HTAB / SP / VCHAR)
   *
   *   The reason-phrase element exists for the sole purpose of providing a
   * textual description associated with the numeric status code, mostly out of
   * deference to earlier Internet application protocols that were more
   * frequently used with interactive text clients.
   */
    void ParseReason(Argument& arg, error_code& ec) {
      static_assert(http_response<Message>);
      const char* reason_beg = arg.buffer_beg + arg.parsed_len;
      for (const char* p = reason_beg; p < arg.buffer_end; ++p) {
        if (*p != '\r') {
          continue;
        }
        if (*(p + 1) != '\n') {
          ec = Error::kBadLineEnding;
          return;
        }
        message_->reason = {reason_beg, p};
        // Skip the "\r\n"
        arg.parsed_len += p - reason_beg + 2;
        message_state_ = MessageState::kExpectingNewline;
        return;
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details Parse a new line. If the line is "\r\n", it means that all the
   *          headers have been parsed and the body can be parsed. Otherwise,
   *          the line is still a header line.
   */
    void ParseExpectingNewLine(Argument& arg, error_code& ec) {
      const char* line_beg = arg.buffer_beg + arg.parsed_len;
      if (arg.buffer_end - line_beg < 2) {
        ec = Error::kNeedMore;
        return;
      }
      if (*line_beg == '\r' && *(line_beg + 1) == '\n') {
        message_state_ = MessageState::kBody;
        arg.parsed_len += 2;
        return;
      }
      message_state_ = MessageState::kHeader;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details: According to RFC 9110:
   *                 header-name    = tokens
   * Parse http header name. A header name labels the corresponding header value
   * as having the semantics defined by that name. Header names are
   * case-insensitive. In order to reduce the performance penalty, the library
   * directly stores the header's lowercase mode, which makes it easier to use
   * the case-insensitive mode. And the header name is not allowed to be empty.
   * Any leading whitespace is aslo not allowed. Note that the header name maybe
   * repeated. In this case, its combined header value consists of the list of
   * corresponding header line values within that section, concatenated in
   * order, with each header line value separated by a comma.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-field-names
   */
    void ParseHeaderName(Argument& arg, error_code& ec) {
      const char* name_beg = arg.buffer_beg + arg.parsed_len;
      for (const char* p = name_beg; p < arg.buffer_end; ++p) {
        if (*p == ':') {
          if (p == name_beg) [[unlikely]] {
            ec = Error::kEmptyHeaderName;
            return;
          }

          // Header name is case-insensitive. Save header name in lowercase.
          if (p - name_beg > name_.size()) {
            name_.resize(p - name_beg);
          }
          std::transform(name_beg, p, name_.begin(), tolower);

          inner_parsed_len_ += p - name_beg + 1;
          header_state_ = HeaderState::kSpacesBeforeValue;
          return;
        }
        if (!detail::IsToken(*p)) {
          ec = Error::kBadHeaderName;
          return;
        }
      }
      ec = Error::kNeedMore;
    }

    /*
   * @brief When the header name is repeated, some header names can simply use
   * the last appeared value, while others can combine all the values into a
   * list as the final value. This function returns whether the given parameter
   * belongs to the second type of header.
   */
    static bool NeedConcatHeaderValue(std::string_view header_name) noexcept {
      return header_name == "accept-encoding";
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                field-value   = *field-content
   *                field-content = field-vchar
   *                                [ 1*( SP / HTAB / field-vchar ) field-vchar]
   *                field-vchar   = VCHAR / obs-text
   *                VCHAR         = any visible US-ASCII character
   *                obs-text      = %x80-FF
   * Parse http header value. HTTP header value consist of a sequence of
   * characters in a format defined by the field's grammar. A header value does
   * not include leading or trailing whitespace. And it's not allowed to be
   * empty. When the name is repeated, the value corresponding to the name is
   * either the last value or a list of all values, separated by commas.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-field-values
   */
    void ParseHeaderValue(Argument& arg, error_code& ec) {
      const char* value_beg = arg.buffer_beg + arg.parsed_len + inner_parsed_len_;
      for (const char* p = value_beg; p < arg.buffer_end; ++p) {
        // Focus only on '\r'.
        if (*p != '\r') {
          continue;
        }

        // Skip the tail whitespaces.
        const char* whitespace = p - 1;
        while (*whitespace == ' ') {
          --whitespace;
        }

        // Empty header value.
        if (whitespace == value_beg - 1) {
          ec = Error::kEmptyHeaderValue;
          return;
        }

        std::string header_value{value_beg, whitespace + 1};

        // Concat all header values in a list.
        if (NeedConcatHeaderValue(name_) && message_->headers.contains(name_)) {
          message_->headers[name_] += "," + header_value;
        } else {
          message_->headers[name_] = header_value;
        }
        inner_parsed_len_ += p - value_beg;
        header_state_ = HeaderState::kHeaderLineEnding;
        return;
      }
      ec = Error::kNeedMore;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details Parse spaces between header name and header value.
   */
    void ParseSpacesBeforeHeaderValue(Argument& arg, error_code& ec) {
      const char* spaces_beg = arg.buffer_beg + arg.parsed_len + inner_parsed_len_;
      const char* p = detail::TrimFront(spaces_beg, arg.buffer_end);
      if (p == arg.buffer_end) {
        ec = Error::kNeedMore;
        return;
      }
      inner_parsed_len_ += p - spaces_beg;
      header_state_ = HeaderState::kValue;
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *            header line ending = "\r\n"
   * Parse http header line ending.
   */
    void ParseHeaderLineEnding(Argument& arg, error_code& ec) {
      const char* line_ending_beg = arg.buffer_beg + arg.parsed_len + inner_parsed_len_;
      if (arg.buffer_end - line_ending_beg < 2) {
        ec = Error::kNeedMore;
        return;
      }
      if (*line_ending_beg != '\r' || *(line_ending_beg + 1) != '\n') {
        ec = Error::kBadLineEnding;
        return;
      }

      // After a completed header line successfully parsed, parse some special
      // headers.
      ParseSpecialHeaders(name_, ec);
      if (ec) {
        return;
      }
      arg.parsed_len += inner_parsed_len_ + 2;
      name_.clear();
      inner_parsed_len_ = 0;
      header_state_ = HeaderState::kName;
      message_state_ = MessageState::kExpectingNewline;
    }

    // Note that header name must be lowecase.
    void ParseSpecialHeaders(const std::string& header_name, error_code& ec) {
      if (header_name == "content-length") {
        return ParseHeaderContentLength(ec);
      }

      if (header_name == "connection") {
        return ParseHeaderConnection(ec);
      }
    }

    void ParseHeaderConnection(error_code& ec) {
    }

    void ParseHeaderContentLength(error_code& ec) {
      if (!message_->headers.contains("content-length")) {
        message_->content_length = 0;
      }

      std::string_view length_string = message_->headers["content-length"];
      std::size_t length{0};
      auto [_, res] = std::from_chars(length_string.begin(), length_string.end(), length);
      if (res == std::errc()) {
        message_->content_length = length;
      } else {
        ec = Error::kBadContentLength;
      }
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details Parse http headers. HTTP headers let the client and the server
   * pass additional information with an HTTP request or response. An HTTP
   * header consists of it's case-insensitive name followed by a colon (:), then
   * by its value. Whitespace before the value is ignored. Note that the header
   * name maybe repeated.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-fields
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-field-syntax
   */
    void ParseHeader(Argument& arg, error_code& ec) {
      inner_parsed_len_ = 0;
      header_state_ = HeaderState::kName;
      while (!ec) {
        switch (header_state_) {
        case HeaderState::kName: {
          ParseHeaderName(arg, ec);
          break;
        }
        case HeaderState::kSpacesBeforeValue: {
          ParseSpacesBeforeHeaderValue(arg, ec);
          break;
        }
        case HeaderState::kValue: {
          ParseHeaderValue(arg, ec);
          break;
        }
        case HeaderState::kHeaderLineEnding: {
          ParseHeaderLineEnding(arg, ec);
          return;
        }
        } // switch
      }   // while
    }

    /*
   * @param arg Records information about the currently parsed buffer.
   * @param ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                  message-body = *OCTET
   *                         OCTET =  any 8-bit sequence of data
   *
   *   Parse http header request body. The message body (if any) of an HTTP/1.1
   * message is used to carry content for the request or response. The message
   * body is identical to the content unless a transfer coding has been applied.
   * The rules for determining when a message body is present in an HTTP/1.1
   * message differ for requests and responses.
   *   1. The presence of a message body in a REQUEST is signaled by a
   * Content-Length or Transfer-Encoding header field. Request message framing
   * is independent of method semantics.
   *   2. The presence of a message body in a RESPONSE depends on both the
   * request method to which it is responding and the response status code. This
   * corresponds to when response content is allowed by HTTP semantics
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-message-body
   */
    //  WARN: Currently this library only deal with Content-Length case.
    void ParseBody(Argument& arg, error_code& ec) {
      if (message_->content_length == 0) {
        message_state_ = MessageState::kCompleted;
        return;
      }
      const char* body_beg = arg.buffer_beg + arg.parsed_len;
      if (arg.buffer_end - body_beg < message_->content_length) {
        ec = Error::kNeedMore;
        return;
      }
      message_->body.reserve(message_->content_length);
      message_->body = {body_beg, body_beg + message_->content_length};
      arg.parsed_len += message_->content_length;
      message_state_ = MessageState::kCompleted;
    }

    // States.
    MessageState message_state_{MessageState::kNothingYet};
    RequestLineState request_line_state_{RequestLineState::kMethod};
    StatusLineState status_line_state_{StatusLineState::kVersion};
    UriState uri_state_{UriState::kInitial};
    ParamState param_state_{ParamState::kName};
    HeaderState header_state_{HeaderState::kName};

    // Records the length of the buffer currently parsed, for URI and headers
    // only. Because both the URI and header contain multiple subparts, different
    // subparts are parsed by different parsing functions. When parsing a subpart,
    // we cannot update the parsed_len of the Argument directly, because we do
    // not know whether the URI or header can be successfully parsed in the end.
    // However, the subsequent function needs to know the location of the buffer
    // that the previous function  parsed, which is recorded by this variable.
    uint32_t inner_parsed_len_{0};

    // Record the parameter name or header name.
    std::string name_;

    // Inner request. The request pointer must be made available during parser
    // parsing.
    // TODO: use unique_ptr
    Message* message_{nullptr};
  };

  using RequestParser = MessageParser<http1::Request>;
  using ResponseParser = MessageParser<http1::Response>;

} // namespace net::http1
