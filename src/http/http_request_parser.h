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
#include <cctype>
#include <concepts>
#include <limits>
#include <span>
#include <string>
#include <tuple>
#include <utility>

#include "http/http_common.h"
#include "http/http_error.h"
#include "http/http_request.h"

// TODO(xiaoming): Refactor all documents. Remove the text we copied.

namespace net::http {
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 16
    0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0,  // 32
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  // 48
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 64
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,  // 80
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 96
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,  // 112
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 128
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 144
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 160
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 176
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 192
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 208
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 224
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 240-255
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
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  16
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //  32
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //  48
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //  64
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //  80
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //  96
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  // 112
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 128
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 144
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 160
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 176
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 192
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 208
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 224
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1   // 240
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
constexpr std::string_view ToString(const char* first,
                                    const char* last) noexcept {
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
inline bool Compare4Char(const char* p, char c0, char c1, char c2,
                         char c3) noexcept {
  return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3;
}

// Helper function to compare 5 characters.
inline bool Compare5Char(const char* p, char c0, char c1, char c2, char c3,
                         char c4) noexcept {
  return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4;
}

// Helper function to compare 6 characters.
inline bool Compare6Char(const char* p, char c0, char c1, char c2, char c3,
                         char c4, char c5) noexcept {
  return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4 &&
         p[5] == c5;
}

// Helper function to compare 7 characters.
inline bool Compare7Char(const char* p, char c0, char c1, char c2, char c3,
                         char c4, char c5, char c6) noexcept {
  return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4 &&
         p[5] == c5 && p[6] == c6;
}

// Helper function to compare 8 characters.
inline bool Compare8Char(const char* p, char c0, char c1, char c2, char c3,
                         char c4, char c5, char c6, char c7) noexcept {
  return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3 && p[4] == c4 &&
         p[5] == c5 && p[6] == c6 && p[7] == c7;
}

// Helper function to compare "http" characters in case-sensitive mode.
inline bool CaseCompareHttpChar(const char* p) {
  return (p[0] | 0x20) == 'h' && (p[1] | 0x20) == 't' && (p[2] | 0x20) == 't' &&
         (p[3] | 0x20) == 'p';
}

// Helper function to compare "https" characters in case-sensitive mode.
inline bool CaseCompareHttpsChar(const char* p) {
  return (p[0] | 0x20) == 'h' && (p[1] | 0x20) == 't' && (p[2] | 0x20) == 't' &&
         (p[3] | 0x20) == 'p' && (p[4] | 0x20) == 's';
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

}  // namespace detail

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

class RequestParser {
 public:
  explicit RequestParser(RequestBase* request) noexcept : request_(request) {}

  void Reset(RequestBase* request) noexcept {
    request_ = request;
    Reset();
  }

  void Reset() noexcept {
    state_ = State::kInitial;
    header_state_ = HeaderState::kName;
    uri_state_ = UriState::kInitial;
    uri_parsed_len_ = 0;
  }

  std::size_t Parse(std::span<char> buffer, error_code& ec) {
    assert(
        !ec &&
        "RequestParser::parse() failed. The parameter ec should be clear.\n");
    const char* p = buffer.data();
    const char* end = p + buffer.size();
    while (!ec) {
      switch (state_) {
        case State::kInitial:
          state_ = State::kMethod;
        case State::kMethod: {
          ParseMethod(p, end, ec);
          break;
        }
        case State::kSpacesBeforeUri: {
          ParseSpacesBeforeUri(p, end, ec);
          break;
        }
        case State::kUri: {
          ParseUri(p, end, ec);
          break;
        }
        case State::kSpacesBeforeHttpVersion: {
          ParseSpacesBeforeVersion(p, end, ec);
          break;
        }
        case State::kVersion: {
          ParseVersion(p, end, ec);
          break;
        }
        case State::kExpectingNewline: {
          ParseExpectingNewLine(p, end, ec);
          break;
        }
        case State::kHeader: {
          ParseHeader(p, end, ec);
          break;
        }
        case State::kBody: {
          ParseBody(p, end, ec);
          break;
        }
        case State::kCompleted: {
          return end - p;
        }
      }
    }
    // If fatal error occurs, return zero as parsed size.
    return ec == Error::kNeedMore ? p - buffer.data() : 0;
  }

 private:
  enum class State {
    kInitial,
    kMethod,
    kSpacesBeforeUri,
    kUri,
    kSpacesBeforeHttpVersion,
    kVersion,
    kHeader,
    kBody,
    kExpectingNewline,
    kCompleted
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
    kSpacesAfterValue,
    kCR,
    kLF,
  };

  enum class ParamState { kName, kValue, kCompleted };

  /*
   * @brief Parse http request method.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details According to RFC 9112: method = token
   *          The method token is case-sensitive. By convention, standardized
   *          methods are defined in all-uppercase US-ASCII letters. If no error
   *          occurred, the method field of inner request will be filled and the
   *          state_ of this parser will be changed from kMethod to
   *          kSpacesBeforeUri.
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-method
   */
  void ParseMethod(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      // Collect method string until met first non-method charater.
      if (detail::IsToken(*p)) [[likely]] {
        continue;
      }

      // Empty method is not allowed. And the first charater after method must
      // be whitespace.
      if (*p != ' ' || p == beg) {
        ec = Error::kBadMethod;
        return;
      }

      request_->method = detail::ToHttpMethod(beg, p);
      if (request_->method == HttpMethod::kUnknown) {
        ec = Error::kBadMethod;
        return;
      }

      beg = p;
      state_ = State::kSpacesBeforeUri;
      return;
    }
    ec = Error::kNeedMore;
  }

  /*
   * @brief Parse white spaces before http scheme.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details Multiply whitespaces are allowd between method and URI.
   */
  void ParseSpacesBeforeUri(const char*& beg, const char* end, error_code& ec) {
    const char* p = detail::TrimFront(beg, end);
    if (p == end) [[unlikely]] {
      ec = Error::kNeedMore;
      return;
    }
    beg = p;
    state_ = State::kUri;
  }

  /*
   * @brief Parse http request uri.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details The "http" and "https" URI defined as below:
   *                     http-URI  = "http"  "://" authority path [ "?" query ]
   *                     https-URI = "https" "://" authority path [ "?" query ]
   *                     authority = host ":" port
   *          Note that we will use the first non-whitespace charater to judge
   *          the URI form. That's to say, if the URI starts with '/', the URI
   *          form is absolute-path. Otherwise it's absolute-form. You may need
   *          to read RFC9112 to get a detailed difference with these two forms.
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-request-target
   */
  void ParseUri(const char*& beg, const char* end, error_code& ec) noexcept {
    if (beg == end) {
      ec = Error::kNeedMore;
      return;
    }

    // Don't always parse from the beginning of uri buffer.
    const char* p = beg + uri_parsed_len_;
    while (!ec) {
      switch (uri_state_) {
        case UriState::kInitial: {
          if (*p == '/') {
            request_->port = 80;
            uri_state_ = UriState::kPath;
          } else {
            uri_state_ = UriState::kScheme;
          }
          break;
        }
        case UriState::kScheme: {
          ParseScheme(p, end, ec);
          break;
        }
        case UriState::kHost: {
          ParseHost(p, end, ec);
          break;
        }
        case UriState::kPort: {
          ParsePort(p, end, ec);
          break;
        }
        case UriState::kPath: {
          ParsePath(p, end, ec);
          break;
        }
        case UriState::kParams: {
          ParseParams(p, end, ec);
          break;
        }
        case UriState::kCompleted: {
          state_ = State::kSpacesBeforeHttpVersion;
          uri_parsed_len_ = p - beg;
          ::memcpy(request_->uri.data(), beg, p - beg);
          request_->uri_len = p - beg;
          beg = p;
          return;
        }
      }
    }
    uri_parsed_len_ = p - beg;
  }

  /*
   * @brief Parse http request scheme.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                        scheme = "+" | "-" | "." | DIGIT | ALPHA
   *          The scheme are case-insensitive and normally provided in
   *          lowercase. In this library, we mainly parse two schemes: "http"
   *          URI scheme and "https" URI scheme.  If no error occurs, the inner
   *          request's scheme field will be filled.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
   */
  void ParseScheme(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      // Skip the scheme character to find first colon.
      if (((*p | 0x20) >= 'a' && (*p | 0x20) <= 'z') ||  //
          (*p >= '0' && *p <= '9') ||                    //
          (*p == '+' || *p == '-' || *p == '.')) {
        continue;
      }

      // The length of scheme name must be more than 2.
      // The character next to the first colon must be "//".
      if (*p != ':' || end - p < 2 || *(p + 1) != '/' || *(p + 2) != '/') {
        ec = Error::kBadScheme;
        return;
      }

      if (p - beg >= 5 && detail::CaseCompareHttpChar(beg)) {
        request_->scheme = HttpScheme::kHttps;
      } else if (p - beg >= 4 && detail::CaseCompareHttpChar(beg)) {
        request_->scheme = HttpScheme::kHttp;
      } else {
        request_->scheme = HttpScheme::kUnknown;
      }
      beg = p + 3;
      uri_state_ = UriState::kHost;
      return;
    }
    ec = Error::kNeedMore;
  }

  /*
   * @brief Parse http request host.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                          host = "-" | "." | DIGIT | ALPHA
   *          Parse http request host identifier. The host can be IP address
   *          style ASCII string (both IPv4 address and IPv6 address) or a
   *          domain name ASCII string which could be DNS resolved future.
   *          Note that empty host identifier is not allowed in a "http" or
   *          "https" URI scheme. If no error occurs, the inner request's host
   *          field will be filled.
   * @see
   * https://datatracker.ietf.org/doc/html/rfc9110#name-authoritative-access
   * https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
   */
  void ParseHost(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      // Collect host string until met first non-host charater.
      if (((*p | 0x20) >= 'a' && (*p | 0x20) <= 'z') ||  //
          (*p >= '0' && *p <= '9') ||                    //
          (*p == '-' || *p == '.')) {
        continue;
      }

      // The charater after host must be follows.
      if (*p != ':' && *p != '/' && *p != '?' && *p != ' ') {
        ec = Error::kBadHost;
        return;
      }

      // A empty host is not allowed at "http" scheme and "https" scheme.
      if (p == beg && (request_->scheme == HttpScheme::kHttp ||
                       request_->scheme == HttpScheme::kHttps)) {
        ec = Error::kBadHost;
        return;
      }

      // Fill host field of inner request.
      request_->host_len = p - beg;
      ::memcpy(request_->host.data(), beg, p - beg);

      // Go to next state depends on the first charater after host identifier.
      switch (*p) {
        case ':':
          beg = p + 1;
          uri_state_ = UriState::kPort;
          return;
        case '/':
          request_->port = detail::GetDefaultPortByHttpScheme(request_->scheme);
          beg = p;
          uri_state_ = UriState::kPath;
          return;
        case '?':
          request_->port = detail::GetDefaultPortByHttpScheme(request_->scheme);
          beg = p + 1;
          uri_state_ = UriState::kParams;
          return;
        case ' ':
          request_->port = detail::GetDefaultPortByHttpScheme(request_->scheme);
          beg = p;
          uri_state_ = UriState::kCompleted;
          return;
      }
    }
    ec = Error::kNeedMore;
  }

  /*
   * @brief Parse http request port.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code hich holds detailed failure reason.
   * @details According to RFC 9110:
   *                          port = DIGIT
   *          This function is used to parse http request port. Http request
   *          port is a digit charater collection which used for TCP connection.
   *          So the port value is less than 65535 in most Unix like platforms.
   *          And according to RFC 9110, port can have leading zeros. If port is
   *          elided from the URI, the default port for that scheme is used. If
   *          no error occurs, the inner request's port field will be filled.
   * @see
   * https://datatracker.ietf.org/doc/html/rfc9110#name-authoritative-access
   */
  void ParsePort(const char*& beg, const char* end, error_code& ec) {
    uint16_t acc = 0;
    uint16_t cur = 0;

    // Always parse port from the beginning of the given buffer since use
    // another variable to cache accumulate value makes thing comlicated while
    // only save no more than five charater's parse time.
    for (const char* p = beg; p < end; ++p) {
      if (*p < '0' || *p > '9') [[unlikely]] {
        if (*p == '/') {
          request_->port =
              acc != 0 ? acc
                       : detail::GetDefaultPortByHttpScheme(request_->scheme);
          beg = p;
          uri_state_ = UriState::kPath;
        } else if (*p == '?') {
          request_->port =
              acc != 0 ? acc
                       : detail::GetDefaultPortByHttpScheme(request_->scheme);
          beg = p + 1;
          uri_state_ = UriState::kParams;
        } else if (*p == ' ') {
          request_->port =
              acc != 0 ? acc
                       : detail::GetDefaultPortByHttpScheme(request_->scheme);
          beg = p;
          uri_state_ = UriState::kCompleted;
        } else {
          ec = Error::kBadPort;
        }
        return;
      }

      // Calculate the port value.
      cur = *p - '0';
      if (acc * 10 + cur > std::numeric_limits<uint16_t>::max()) {
        ec = Error::kBadPort;
        return;
      }
      acc = acc * 10 + cur;
    }
    ec = Error::kNeedMore;
  }

  /*
   * @brief Parse http request path.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *                          path = token
   *          Http request path is defined as the path and filename in a
   *          request. It does not include scheme, host, port or query string.
   *          If no error occurs, the inner request's path field will be filled.
   */
  void ParsePath(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      if (*p == '?') {
        ::memcpy(request_->path.data(), beg, p - beg);
        request_->path_len = p - beg;
        beg = p + 1;
        uri_state_ = UriState::kParams;
        return;
      }
      if (*p == ' ') {
        ::memcpy(request_->path.data(), beg, p - beg);
        request_->path_len = p - beg;
        beg = p;
        uri_state_ = UriState::kCompleted;
        return;
      }
      if (!detail::IsPathChar(*p)) {
        ec = Error::kBadPath;
        return;
      }
    }
    ec = Error::kNeedMore;
  }

  void ParseParamName(const char*& beg, const char* end, error_code& ec) {
    const char* name_start = beg;
    // Skip the beginning of the continuous '&', which represents empty
    // parameter name and parameter value.
    while (name_start < end && *name_start == '&') {
      ++name_start;
    }
    if (name_start == end) {
      ec = Error::kNeedMore;
      return;
    }
    for (const char* p = name_start; p < end; ++p) {
      if (*p == '&') {
        // When all characters in two ampersand don't contain =, all
        // characters in ampersand (except ampersand) are considered
        // parameter names.
        request_->params.emplace(std::string(name_start, p), "");
        beg = p + 1;
        return;
      }
      if (*p == '=') {
        ::memcpy(temp_buffer_.data(), name_start, p - name_start);
        temp_len_ = p - name_start;
        // Skip the equal mark.
        beg = p + 1;
        param_state_ = ParamState::kValue;
        return;
      }
      if (*p == ' ') {
        // When there are at least one characters in '&' and ' ', trated
        // them as parameter name.
        if (name_start != p) {
          request_->params.emplace(std::string(name_start, p), "");
        }
        beg = p;
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

  void ParseParamValue(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      if (*p == '&') {
        request_->params.emplace(std::string(temp_buffer_.data(), temp_len_),
                                 std::string{beg, p});
        beg = p + 1;
        param_state_ = ParamState::kName;
        return;
      }
      if (*p == ' ') {
        request_->params.emplace(std::string(temp_buffer_.data(), temp_len_),
                                 std::string{beg, p});
        beg = p;
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
   * @brief Parse http request parameters.
   * @param[in]  beg A pointer to the beginning of the data to be parsed.
   * @param[in]  end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details According to RFC 9110:
   *               parameters      = *( OWS ";" OWS [ parameter ] )
   *               parameter       = parameter-name "=" parameter-value
   *               parameter-name  = token
   *               parameter-value = ( token / quoted-string )
   *               OWS             = *( SP / HTAB )
   *                               ; optional whitespace
   * The parameter name is allowed to be empty only when explicitly set the
   * equal mark. In all other situations, an empty parameter name is
   * forbiddened. Note that there isn't a clear specification illustrate what to
   * do when there is a repeated key, we just use the first-occur policy now. If
   * no error occurred, the params field of inner request will be filled and the
   * state_ of this parser will be changed from kParams to
   * kSpacesBeforeHttpVersion.
   * @see parameters:
   *           https://datatracker.ietf.org/doc/html/rfc9110#name-parameters
   * @see quoted-string:
   *           https://datatracker.ietf.org/doc/html/rfc9110#name-quoted-strings
   * @see parse query parameter
   *           https://url.spec.whatwg.org/#query-state
   */
  void ParseParams(const char*& beg, const char* end, error_code& ec) {
    while (!ec) {
      switch (param_state_) {
        case ParamState::kName: {
          ParseParamName(beg, end, ec);
          break;
        }
        case ParamState::kValue: {
          ParseParamValue(beg, end, ec);
          break;
        }
        case ParamState::kCompleted: {
          temp_len_ = 0;
          uri_state_ = UriState::kCompleted;
          return;
        }
      }
    }
  }

  /*
   * @brief Parse white spaces before http version.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details Multiply whitespaces are allowd between uri and http version.
   */
  void ParseSpacesBeforeVersion(const char*& beg, const char* end,
                                error_code& ec) {
    const char* p = detail::TrimFront(beg, end);
    if (p == end) [[unlikely]] {
      ec = Error::kNeedMore;
      return;
    }
    beg = p;
    state_ = State::kVersion;
  }

  /*
   * @brief Parse http request version.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details HTTP's version number consists of two decimal digits separated by
   *          a "." (period or decimal point). The first digit (major version)
   *          indicates the messaging syntax, whereas the second digit (minor
   *          version) indicates the highest minor version within that major
   *          version to which the sender is conformant (able to understand for
   *          future communication).
   *          According to RFC 9112:
   *                 HTTP-version  = HTTP-name "/" DIGIT "." DIGIT
   *                 HTTP-name     = %s"HTTP"
   *          Addition to http version itself, we also need "\r\n" to parse
   *          together.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-protocol-version
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-http-version
   */
  void ParseVersion(const char*& beg, const char* end, error_code& ec) {
    if (end - beg < 10) {
      ec = Error::kNeedMore;
      return;
    }
    if (beg[0] != 'H' ||              //
        beg[1] != 'T' ||              //
        beg[2] != 'T' ||              //
        beg[3] != 'P' ||              //
        beg[4] != '/' ||              //
        std::isdigit(beg[5]) == 0 ||  //
        beg[6] != '.' ||              //
        std::isdigit(beg[7]) == 0 ||  //
        beg[8] != '\r' ||             //
        beg[9] != '\n') {
      ec = Error::kBadVersion;
      return;
    }

    request_->version = ToHttpVersion(beg[5] - '0', beg[7] - '0');
    beg += 10;
    state_ = State::kExpectingNewline;
  }

  /*
   * @brief Parse a new line.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details Parse a new line. If the line is "\r\n", it means that all the
   *          headers have been parsed and the body can be parsed. Otherwise,
   *          the line is still a header line.
   */
  void ParseExpectingNewLine(const char*& beg, const char* end,
                             error_code& ec) {
    if (end - beg < 2) {
      ec = Error::kNeedMore;
      return;
    }
    if (*beg == '\r' && *(beg + 1) == '\n') {
      state_ = State::kBody;
      beg += 2;
      return;
    }
    state_ = State::kHeader;
  }

  void ParseHeaderName(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      if (*p == ':') {
        if (p == beg) {
          // empty header name
          ec = Error::kBadHeaderName;
          return;
        }
        ::memcpy(temp_buffer_.data(), beg, p - beg);
        temp_len_ = p - beg;
        // Skip the colon.
        beg = p + 1;
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

  void ParseHeaderValue(const char*& beg, const char* end, error_code& ec) {
    for (const char* p = beg; p < end; ++p) {
      if (*p == ' ' || *p == '\r' || *p == '\n') {
        request_->headers.emplace(std::string{temp_buffer_.data(), temp_len_},
                                  detail::ToString(beg, p));
        temp_len_ = 0;
        beg = p;
        switch (*p) {
          case ' ': {
            header_state_ = HeaderState::kSpacesAfterValue;
            return;
          }
          case '\r': {
            header_state_ = HeaderState::kCR;
            return;
          }
          case '\n': {
            header_state_ = HeaderState::kLF;
            return;
          }
        }
      }
      // TODO() Check valid value characters.
    }
    ec = Error::kNeedMore;
  }

  /*
   * @brief Parse http headers.
   * @param[in] beg A pointer to the beginning of the data to be parsed.
   * @param[in] end A Pointer to the next position from the end of the buffer.
   * @param[out] ec The error code which holds detailed failure reason.
   * @details HTTP headers let the client and the server pass additional
   *          information with an HTTP request or response. An HTTP header
   *          consists of it's case-insensitive name followed by a colon (:),
   *          then by its value. Whitespace before the value is ignored.
   *          According to RFC 9110:
   *                header-name   = token
   *                field-value   = *field-content
   *                field-content = field-vchar
   *                                [ 1*( SP / HTAB / field-vchar ) field-vchar]
   *                field-vchar   = VCHAR / obs-text
   *                VCHAR         = any visible US-ASCII character
   *                obs-text      = %x80-FF
   *          Note that the header name maybe repeated. In this case, its
   *          combined header value consists of the list of corresponding header
   *          line values within that section, concatenated in order, with each
   *          header line value separated by a comma.
   * @see https://datatracker.ietf.org/doc/html/rfc9110#name-fields
   * @see https://datatracker.ietf.org/doc/html/rfc9112#name-field-syntax
   */
  void ParseHeader(const char*& beg, const char* end, error_code& ec) {
    while (!ec) {
      switch (header_state_) {
        case HeaderState::kName: {
          ParseHeaderName(beg, end, ec);
          break;
        }
        case HeaderState::kSpacesBeforeValue: {
          detail::TrimFront(beg, end);
          break;
        }
        case HeaderState::kValue: {
          ParseHeaderValue(beg, end, ec);
          break;
        }
        case HeaderState::kSpacesAfterValue: {
          beg = detail::TrimFront(beg, end);
          if (beg == end) {
            ec = Error::kNeedMore;
            break;
          }
          if (*beg == '\r') {
            header_state_ = HeaderState::kLF;
            ++beg;
            break;
          }
          ec = Error::kBadLineEnding;
          return;
        }
        case HeaderState::kCR: {
          if (beg == end) {
            ec = Error::kNeedMore;
            break;
          }
          if (*beg == '\r') {
            header_state_ = HeaderState::kLF;
            ++beg;
            break;
          }
          ec = Error::kBadLineEnding;
          return;
        }
        case HeaderState::kLF: {
          if (beg == end) {
            ec = Error::kNeedMore;
            break;
          }
          if (*beg == '\n') {
            beg = beg + 1;
            header_state_ = HeaderState::kName;
            state_ = State::kExpectingNewline;
            return;
          }
          ec = Error::kBadLineEnding;
          return;
        }
      }  // switch
    }    // while
  }

  void ParseBody(const char*& beg, const char* end, error_code& ec) {}

  State state_{State::kInitial};
  UriState uri_state_{UriState::kInitial};
  std::size_t uri_parsed_len_{0};
  ParamState param_state_{ParamState::kName};
  HeaderState header_state_{HeaderState::kName};
  RequestBase* request_{nullptr};
  uint32_t temp_len_{0};
  std::array<char, 8192> temp_buffer_{};
};

}  // namespace net::http
