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

#include <cstring>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <limits>
#include <string>
#include <system_error>
#include <memory>

#include <fmt/core.h>

#include "expected.h"
#include "buffer.h"
#include "http/http_common.h"
#include "http/http_concept.h"
#include "http/http_error.h"

// TODO: Still need to implement and optimize this parser. Write more
// test case to make this parser robust. This is a long time work, we need to
// implement others first.
// TODO: Add some UTF-8 test cases. We should support theses encoding type: Latin1, UTF-8, ASCII,
//       especially UTF-8.
// TODO: support chunk

namespace net::http::http1 {
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
    constexpr std::array<std::uint8_t, 256> tokens = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //   0-15
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  16-31
      0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, //  32-47
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, //  48-63
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  64-79
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, //  80-95
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  96-111
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, // 112-127
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 128-143
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 144-159
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 160-175
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 176-191
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 192-207
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 208-223
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 224-239
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
     * @param   b The charater to be checked.
    */
    inline bool is_token(std::byte b) noexcept {
      return static_cast<bool>(tokens[std::to_integer<uint8_t>(b)]);
    }

    constexpr std::array<uint8_t, 256> uri_characters = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //   0-15
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  16-31
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  32-47
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  48-63
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  64-79
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  80-95
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  96-111
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // 112-127
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 128-143
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 144-159
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 160-175
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 176-191
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 192-207
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 208-223
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 224-239
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // 240-255
    };

    /**
     * @brief Check whether a character meets the requirement of HTTP URI.
     * @param b the charater to be checked.
    */
    inline bool is_uri_char(std::byte b) noexcept {
      return static_cast<bool>(uri_characters[std::to_integer<uint8_t>(b)]);
    }

    /*
     * @brief  Skip the whitespace from front.
     * @param  first The start address of the data.
     * @param  last  The end address of the data. The data range is half-opened.
     * @return The pointer which points to the first non-whitespace position or "last"
     *         which represents all charater between [first, last) is whitespace.
    */
    inline const std::byte* trim_front(const std::byte* first, const std::byte* last) noexcept {
      while (first < last && (*first == std::byte{' '} || *first == std::byte{'\t'})) {
        ++first;
      }
      return first;
    }

    // Calculate the length between two pointers.
    inline std::size_t len(const std::byte* beg, const std::byte* end) noexcept {
      return end - beg;
    }

    inline std::string to_string(const std::byte* beg, const std::byte* end) noexcept {
      return {reinterpret_cast<const char*>(beg), reinterpret_cast<const char*>(end)};
    }

    inline std::string to_string(const std::byte* beg, std::size_t len) noexcept {
      return {reinterpret_cast<const char*>(beg), reinterpret_cast<const char*>(beg + len)};
    }

    inline std::string_view to_string_view(const std::byte* beg, std::size_t len) noexcept {
      return {reinterpret_cast<const char*>(beg), len};
    }

    inline bool is_alpha(std::byte b) {
      return (b | std::byte{0x20}) >= std::byte{'a'} && (b | std::byte{0x20}) <= std::byte{'z'};
    }

    inline bool is_digit(std::byte b) {
      return (b | std::byte{0x20}) >= std::byte{'0'} && (b | std::byte{0x20}) <= std::byte{'9'};
    }

    inline bool is_alnum(std::byte b) {
      return is_alpha(b) && is_digit(b);
    }

    inline bool byte_is(std::byte b, char expect) noexcept {
      return b == static_cast<std::byte>(expect);
    }

    inline bool one_of(const std::byte b, char c0, char c1) noexcept {
      return b == static_cast<std::byte>(c0) || b == static_cast<std::byte>(c1);
    }

    inline bool one_of(const std::byte b, char c0, char c1, char c2) noexcept {
      return b == static_cast<std::byte>(c0) || b == static_cast<std::byte>(c1)
          || b == static_cast<std::byte>(c2);
    }

    inline bool one_of(const std::byte b, char c0, char c1, char c2, char c3) noexcept {
      return b == static_cast<std::byte>(c0) || b == static_cast<std::byte>(c1)
          || b == static_cast<std::byte>(c2) || b == static_cast<std::byte>(c3);
    }

    inline bool one_of(const std::byte b, char c0, char c1, char c2, char c3, char c4) noexcept {
      return b == static_cast<std::byte>(c0) || b == static_cast<std::byte>(c1)
          || b == static_cast<std::byte>(c2) || b == static_cast<std::byte>(c3)
          || b == static_cast<std::byte>(c4);
    }

    // Helper function to compare 2 characters.
    inline bool compare_2_char(const std::byte* p, char c0, char c1) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1);
    }

    // Helper function to compare 3 characters.
    inline bool compare_3_char(const std::byte* p, char c0, char c1, char c2) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1)
          && p[2] == static_cast<std::byte>(c2);
    }

    // Helper function to compare 4 characters.
    inline bool compare_4_char(const std::byte* p, char c0, char c1, char c2, char c3) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1)
          && p[2] == static_cast<std::byte>(c2) && p[3] == static_cast<std::byte>(c3);
    }

    // Helper function to compare 5 characters.
    inline bool
      compare_5_char(const std::byte* p, char c0, char c1, char c2, char c3, char c4) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1)
          && p[2] == static_cast<std::byte>(c2) && p[3] == static_cast<std::byte>(c3)
          && p[4] == static_cast<std::byte>(c4);
    }

    // Helper function to compare 6 characters.
    inline bool compare_6_char(
      const std::byte* p,
      char c0,
      char c1,
      char c2,
      char c3,
      char c4,
      char c5) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1)
          && p[2] == static_cast<std::byte>(c2) && p[3] == static_cast<std::byte>(c3)
          && p[4] == static_cast<std::byte>(c4) && p[5] == static_cast<std::byte>(c5);
    }

    // Helper function to compare 7 characters.
    inline bool compare_7_char(
      const std::byte* p,
      char c0,
      char c1,
      char c2,
      char c3,
      char c4,
      char c5,
      char c6) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1)
          && p[2] == static_cast<std::byte>(c2) && p[3] == static_cast<std::byte>(c3)
          && p[4] == static_cast<std::byte>(c4) && p[5] == static_cast<std::byte>(c5)
          && p[6] == static_cast<std::byte>(c6);
    }

    // Helper function to compare 8 characters.
    inline bool compare_8_char(
      const std::byte* p,
      char c0,
      char c1,
      char c2,
      char c3,
      char c4,
      char c5,
      char c6,
      char c7) noexcept {
      return p[0] == static_cast<std::byte>(c0) && p[1] == static_cast<std::byte>(c1)
          && p[2] == static_cast<std::byte>(c2) && p[3] == static_cast<std::byte>(c3)
          && p[4] == static_cast<std::byte>(c4) && p[5] == static_cast<std::byte>(c5)
          && p[6] == static_cast<std::byte>(c6) && p[7] == static_cast<std::byte>(c7);
    }

    // Helper function to compare "http" characters in case-sensitive mode.
    inline bool case_compare_http_char(const std::byte* p) {
      return (p[0] | std::byte{0x20}) == std::byte{'h'}
          && (p[1] | std::byte{0x20}) == std::byte{'t'}
          && (p[2] | std::byte{0x20}) == std::byte{'t'}
          && (p[3] | std::byte{0x20}) == std::byte{'p'};
    }

    // Helper function to compare "https" characters in case-sensitive mode.
    inline bool case_compare_https_char(const std::byte* p) {
      return (p[0] | std::byte{0x20}) == std::byte{'h'}
          && (p[1] | std::byte{0x20}) == std::byte{'t'}
          && (p[2] | std::byte{0x20}) == std::byte{'t'}
          && (p[3] | std::byte{0x20}) == std::byte{'p'}
          && (p[4] | std::byte{0x20}) == std::byte{'s'};
    }

    inline http_method to_http_method(const std::byte* beg, const std::byte* end) {
      const std::size_t len = end - beg;
      if (len == 3) {
        if (compare_3_char(beg, 'G', 'E', 'T')) {
          return http_method::get;
        }
        if (compare_3_char(beg, 'P', 'U', 'T')) {
          return http_method::put;
        }
      } else if (len == 4) {
        if (compare_4_char(beg, 'P', 'O', 'S', 'T')) {
          return http_method::post;
        }
        if (compare_4_char(beg, 'H', 'E', 'A', 'D')) {
          return http_method::head;
        }
      } else if (len == 5) {
        if (compare_5_char(beg, 'T', 'R', 'A', 'C', 'E')) {
          return http_method::trace;
        }
        if (compare_5_char(beg, 'P', 'U', 'R', 'G', 'E')) {
          return http_method::purge;
        }
      } else if (len == 6) {
        if (compare_6_char(beg, 'D', 'E', 'L', 'E', 'T', 'E')) {
          return http_method::del;
        }
      } else if (len == 7) {
        if (compare_7_char(beg, 'O', 'P', 'T', 'I', 'O', 'N', 'S')) {
          return http_method::options;
        }

        if (compare_7_char(beg, 'C', 'O', 'N', 'T', 'R', 'O', 'L')) {
          return http_method::control;
        }
        if (compare_7_char(beg, 'C', 'O', 'N', 'N', 'E', 'C', 'T')) {
          return http_method::connect;
        }
      }
      return http_method::unknown;
    }

  } // namespace detail

  /**
   * @brief   Indicates current parser's state.
   * @details Data may need to be parsed several times before it becomes a
   * complete and properly formed message. This enum describes the current
   * parsed status.
   * Specifically,
   *    - nothing_yet        no data has been parsed by the current parser.
   *    - start_line         parsing the first line of the message.
   *    - expecting_new_line a new line is required. The new line must be a header or "\r\n".
   *    - header             parsing the message field.
   *    - body               parsing that the message content.
   *    - completed          parsing is completed, a correctly formatted message is generated.
  */
  enum class http1_parse_state {
    nothing_yet,
    start_line,
    expecting_newline,
    header,
    body,
    completed
  };

  /*
   * @details
   * Introduction:
   *   This parser is used for parse HTTP/1.0 and HTTP/1.1 message. According to
   * RFC9112, A HTTP/1.1 message consists of a start-line followed by a CRLF and
   * a sequence of octets in a format similar to the Internet Message Format:
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
   * URI:
   *   A URI consists of many parts. In general, a URI can contain at most scheme,
   * host, port, path, query parameters, and fragment. Each subpart of the URI is
   * described in detail in the corresponding parsing function. For URI parse, in
   * addition to the RFC, I also relied heavily on whatwg standards. The relevant
   * reference links are given below.
   * @see whatwg URL specification: https://url.spec.whatwg.org/#urls
   *
   * Code guides:
   *   This Parser uses the parse_xxx function to parse http messages. Protocol
   * message parsing is always error-prone, and this parser tries its best to
   * ensure that the code style of the parsing function is uniform.
   *   1. All parser_xxx functions take three parameters. The first argument is the
   * start of the buffer that this particular parse_xxx function needs to parse,
   * the second argument is the end of the buffer, and the third argument is error
   * code, which contains the specific error message. Note that the first argument
   * is the reference type, which is assigned and returned by the particular
   * parse_xxx function.
   *   2. The correctness of the parsing is ensured by a large number of associated
   * unit tests.
   *   3. TODO
   *
   * @see RFC 9110: https://datatracker.ietf.org/doc/html/rfc9110
   * @see RFC 9112: https://datatracker.ietf.org/doc/html/rfc9112
  */
  template <http1_message_concept Message>
  class message_parser {
    using error_code = std::error_code;
    using byte_t = std::byte;
    using cbyte_t = const std::byte;
    using pointer = byte_t*;
    using cpointer = const byte_t*;

   public:
    explicit message_parser(Message* message) noexcept
      : message_(message) {
      inner_name_.reserve(8192);
    }

    void reset(Message* message) noexcept {
      message_ = message;
      reset();
    }

    void reset() noexcept {
      state_ = http1_parse_state::nothing_yet;
      header_state_ = header_state::name;
      uri_state_ = uri_state::initial;
      param_state_ = param_state::name;
      inner_name_.clear();
    }

    [[nodiscard]] std::unique_ptr<Message> message() const noexcept {
      return message_;
    }

    [[nodiscard]] bool is_completed() const noexcept {
      return state_ == http1_parse_state::completed;
    }

    size_expected parse(const_buffer buffer) noexcept {
      cpointer beg = buffer.data();
      cpointer end = buffer.data() + buffer.size();
      cpointer p = beg;
      size_expected result = 0;
      while (true) {
        switch (state_) {
        case http1_parse_state::nothing_yet:
        case http1_parse_state::start_line: {
          if constexpr (http1_request_concept<Message>) {
            result = parse_request_line(p, end);
          } else {
            result = parse_status_line(p, end);
          }
          break;
        }
        case http1_parse_state::expecting_newline: {
          result = parse_expecting_new_line(p, end);
          break;
        }
        case http1_parse_state::header: {
          result = parse_header(p, end);
          break;
        }
        case http1_parse_state::body: {
          result = parse_body(p, end);
          break;
        }
        case http1_parse_state::completed: {
          return detail::len(beg, p);
        }
        }
        if (!result) {
          if (result.error() == error::need_more) {
            return detail::len(beg, p);
          }
          return result;
        }
        p += *result;
      }
    }

    [[nodiscard]] http1_parse_state state() const noexcept {
      return state_;
    }

   private:
    enum class request_line_state {
      method,
      spaces_before_uri,
      uri,
      spaces_before_http_version,
      version,
      completed
    };

    enum class status_line_state {
      version,
      status_code,
      reason,
      completed
    };

    enum class uri_state {
      initial,
      scheme,
      host,
      port,
      path,
      params,
      completed
    };

    enum class header_state {
      name,
      spaces_before_value,
      value,
      header_line_ending,
      completed
    };

    enum class param_state {
      name,
      value,
      completed
    };

    inline void update_length_and_buffer(
      const std::size_t parsed_sz,
      std::size_t& total_parsed_len,
      const_buffer& data_buffer) noexcept {
      total_parsed_len += parsed_sz;
      data_buffer = data_buffer.subspan(parsed_sz);
    }

    /*
     * @details According to RFC 9112:
     *               request-line   = method SP request-target SP HTTP-version
     *   A request-line begins with a method token, followed by a single space
     * (SP), the request-target, and another single space (SP), and ends with the
     * protocol version.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-request-line
    */
    size_expected parse_request_line(cpointer beg, cpointer end) noexcept {
      size_expected result{};
      cpointer cur = beg;

      while (true) {
        switch (request_line_state_) {
        case request_line_state::method: {
          result = parse_method(cur, end);
          break;
        }
        case request_line_state::spaces_before_uri: {
          result = parse_spaces_before_uri(cur, end);
          break;
        }
        case request_line_state::uri: {
          result = parse_uri(cur, end);
          break;
        }
        case request_line_state::spaces_before_http_version: {
          result = parse_spaces_before_version(cur, end);
          break;
        }
        case request_line_state::version: {
          result = parse_request_http_version(cur, end);
          break;
        }
        case request_line_state::completed: {
          state_ = http1_parse_state::expecting_newline;
          return detail::len(beg, cur);
        }
        }
        if (!result) {
          return result;
        }
        cur += *result;
      }
    }

    /**
     * @details According to RFC 9112:
     *                        method = token
     * The method token is case-sensitive. By convention, standardized methods
     * are defined in all-uppercase US-ASCII letters and not allowd to be
     * empty. If no error occurred, the method field of inner request will be
     * filled and the request_line_state_ of this parser will be changed from
     * method to spaces_before_uri.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-method
     */
    size_expected parse_method(cpointer beg, cpointer end) noexcept {
      for (cpointer p = beg; p < end; ++p) {
        // Collect method string until met first non-method charater.
        if (detail::is_token(*p)) [[likely]] {
          continue;
        }

        // The first character after method string must be whitespace.
        if (detail::byte_is(*p, ' ')) {
          return net::unexpected(make_error_code(error::bad_method));
        }

        // Empty method is not allowed.
        if (p == beg) {
          return net::unexpected(make_error_code(error::empty_method));
        }

        http_method method = detail::to_http_method(beg, p);
        if (method == http_method::unknown) {
          return net::unexpected(make_error_code(error::unknown_method));
        }

        message_->method = method;
        request_line_state_ = request_line_state::spaces_before_uri;
        return detail::len(beg, p);
      }
      return net::unexpected(make_error_code(error::need_more));
    }

    /**
     * @details Parse multiple whitespaces between method and URI.
     */
    size_expected parse_spaces_before_uri(cpointer beg, cpointer end) noexcept {
      cpointer p = detail::trim_front(beg, end);
      if (p == end) [[unlikely]] {
        return net::unexpected(make_error_code(error::need_more));
      }
      request_line_state_ = request_line_state::uri;
      return detail::len(beg, p);
    }

    // TODO: Add percent-encoded.
    /*
     * @details The "http" and "https" URI defined as below:
     *                     http-URI  = "http"  "://" authority path [ "?" query ]
     *                     https-URI = "https" "://" authority path [ "?" query ]
     *                     authority = host ":" port
     * Parse http request uri. Use the first non-whitespace charater to judge the
     * URI form. That's to say, if the URI starts with '/', the URI form is
     * absolute-path. Otherwise it's absolute-form. You may need to read RFC9112
     * to get a detailed difference with these two forms.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-request-target
     * @see https://url.spec.whatwg.org
    */
    size_expected parse_uri(cpointer beg, cpointer end) noexcept {
      if (beg >= end) {
        return net::unexpected(make_error_code(error::need_more));
      }

      std::size_t parsed_size = 0;
      size_expected result = 0;
      cpointer p = beg;

      uri_state_ = uri_state::initial;
      while (true) {
        switch (uri_state_) {
        case uri_state::initial: {
          if (*beg == std::byte{'/'}) {
            message_->port = 80;
            uri_state_ = uri_state::path;
          } else {
            uri_state_ = uri_state::scheme;
          }
          break;
        }
        case uri_state::scheme: {
          result = parse_scheme(p, end);
          break;
        }
        case uri_state::host: {
          result = parse_host(p, end);
          break;
        }
        case uri_state::port: {
          result = parse_port(p, end);
          break;
        }
        case uri_state::path: {
          result = parse_path(p, end);
          break;
        }
        case uri_state::params: {
          result = parse_params(p, end);
          break;
        }
        case uri_state::completed: {
          request_line_state_ = request_line_state::spaces_before_http_version;
          message_->uri = detail::to_string(beg, parsed_size);
          return parsed_size;
        }
        } // switch
        if (!result) {
          return result;
        }
        parsed_size += *result;
        p += *result;
      }
    }

    /*
     * @details According to RFC 9110:
     *                       scheme = "+" | "-" | "." | DIGIT | ALPHA
     * Parse http request scheme. The scheme are case-insensitive and normally
     * provided in lowercase. This library mainly parse two schemes: "http"
     * URI scheme and "https" URI scheme. If no error occurs, the inner request's
     * scheme field will be filled.
     * @see https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
    */
    size_expected parse_scheme(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        // Skip the scheme character to find first colon.
        if (detail::is_alnum(*p) || detail::one_of(*p, '+', '-', '.')) {
          continue;
        }

        // Need more data.
        if (detail::len(p, end) < 3) {
          return net::unexpected(make_error_code(error::need_more));
        }

        // The characters next to the first non-scheme character must be "://".
        if (detail::compare_3_char(p, ':', '/', '/')) {
          return net::unexpected(make_error_code(error::bad_scheme));
        }

        auto scheme_len = detail::len(beg, p);
        if (scheme_len >= 5 && detail::case_compare_https_char(beg)) {
          message_->scheme = http_scheme::https;
        } else if (scheme_len >= 4 && detail::case_compare_http_char(beg)) {
          message_->scheme = http_scheme::http;
        } else {
          message_->scheme = http_scheme::unknown;
        }

        uri_state_ = uri_state::host;
        return scheme_len + 3;
      }
      return net::unexpected(make_error_code(error::need_more));
    }

    // TODO: In the real world, UTF-8 reg-name could also work.
    /*
     * @details According to RFC 9110:
     *                       host = "-" | "." | DIGIT | ALPHA
     * Parse http request host identifier. The host can be IP address style ASCII
     * string (both IPv4 address and IPv6 address) or a domain name ASCII string
     * which could be DNS resolved future. Note that empty host identifier is not
     * allowed in a "http" or "https" URI scheme. If no error occurs, the inner
     * request's host field will be filled.
     * @see
     * https://datatracker.ietf.org/doc/html/rfc9110#name-authoritative-access
     * https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
    */
    size_expected parse_host(cpointer beg, cpointer end) noexcept {
      for (const auto* p = beg; p < end; ++p) {
        // Collect host string until met first non-host charater.
        if (detail::is_alnum(*p) || detail::one_of(*p, '-', '.')) {
          continue;
        }

        // The charater after host must be follows.
        if (!detail::one_of(*p, ':', '/', '?', ' ')) {
          return net::unexpected(make_error_code(error::bad_host));
        }

        // An empty host is not allowed at "http" scheme and "https" scheme.
        if (
          p == beg
          && (message_->scheme == http_scheme::http || message_->scheme == http_scheme::https)) {
          return net::unexpected(make_error_code(error::empty_host));
        }

        // Fill host field of inner request.
        auto host_len = detail::len(beg, p);
        message_->host = detail::to_string(beg, p);

        // Go to next state depends on the first charater after host identifier.
        switch (*p) {
        case std::byte{':'}:
          uri_state_ = uri_state::port;
          return host_len + 1;
        case std::byte{'/'}:
          message_->port = default_port(message_->scheme);
          uri_state_ = uri_state::path;
          return host_len;
        case std::byte{'?'}:
          message_->port = default_port(message_->scheme);
          uri_state_ = uri_state::params;
          return host_len + 1;
        case std::byte{' '}:
          message_->port = default_port(message_->scheme);
          uri_state_ = uri_state::completed;
          return host_len;
        }
      }

      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @details According to RFC 9110:
     *                       port = DIGIT
     * Parse http request port. This function is used to parse http request port.
     * Http request port is a digit charater collection which used for TCP
     * connection. So the port value is less than 65535 in most Unix like
     * platforms. And according to RFC 9110, port can have leading zeros. If port
     * is elided from the URI, the default port for that scheme is used. If no
     * error occurs, the inner request's port field will be filled.
     * @see
     * https://datatracker.ietf.org/doc/html/rfc9110#name-authoritative-access
    */
    size_expected parse_port(cpointer beg, cpointer end) noexcept {
      uint16_t acc = 0;
      uint16_t cur = 0;
      for (const std::byte* p = beg; p < end; ++p) {
        // Calculate the port value and save it in acc variable.
        if (detail::is_digit(*p)) [[likely]] {
          cur = std::to_integer<uint8_t>(*p);
          if (acc * 10 + cur > std::numeric_limits<port_t>::max()) {
            return net::unexpected(make_error_code(error::too_big_port));
          }
          acc = acc * 10 + cur;
          continue;
        }

        // The first character next to port string should be one of follows.
        if (!detail::one_of(*p, '/', '?', ' ')) {
          return net::unexpected(make_error_code(error::bad_port));
        }

        // Port is zero, so use default port value based on scheme.
        if (acc == 0) {
          acc = default_port(message_->scheme);
        }
        message_->port = acc;
        auto port_string_len = detail::len(beg, p);
        switch (*p) {
        case std::byte{'/'}: {
          uri_state_ = uri_state::path;
          return port_string_len;
        }
        case std::byte{'?'}: {
          uri_state_ = uri_state::params;
          return port_string_len + 1;
        }
        case std::byte{' '}: {
          uri_state_ = uri_state::completed;
          return port_string_len;
        }
        } // switch
      }   // for

      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @details According to RFC 9110:
     *                       path = token
     * Parse http request path. Http request path is defined as the path and
     * filename in a request. It does not include scheme, host, port or query
     * string. If no error occurs, the inner request's path field will be filled.
    */
    size_expected parse_path(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        if (detail::byte_is(*p, '?')) {
          message_->path = detail::to_string(beg, p);
          uri_state_ = uri_state::params;
          return detail::len(beg, p) + 1;
        }

        if (detail::byte_is(*p, ' ')) {
          message_->path = detail::to_string(beg, p);
          uri_state_ = uri_state::completed;
          return detail::len(beg, p);
        }

        if (!detail::is_uri_char(*p)) {
          return net::unexpected(make_error_code(error::bad_path));
        }
      }

      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @bried Parse HTTP request parameter name.
     * @details According to RFC 9110:
     *                       parameter-name  = token
     * - case-sensitive: The parameter name should be case-sensitive. However,
     *   the parameters are saved as is. (Many languages, such as Go, Python,
     *   and Java, store parameters this way.)
     * - empty name: The parameter name is allowed to be empty only when
     *   explicitly set the equal mark. In all other situations, an empty
     *   parameter name is forbiddened.
     * - duplicated name: Note that there isn't a clear specification
     *   illustrate what to do when there is a repeated name, we just store all
     *   duplicated parameters name in order.
     * @see parameters: https://datatracker.ietf.org/doc/html/rfc9110#name-parameters
    */
    size_expected parse_param_name(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        if (detail::byte_is(*p, '&')) {
          param_state_ = param_state::name;

          // Skip the "?&" or continuous '&' , which represents empty
          // parameter name and parameter value.
          if (p == beg || p - beg == 1) {
            return 1;
          }

          // When all characters in two ampersand don't contain =, all
          // characters in ampersand (except ampersand) are considered
          // parameter names.
          inner_name_ = detail::to_string(beg, p);
          message_->params.emplace(inner_name_, "");
          return detail::len(beg, p) + 1;
        }

        if (detail::byte_is(*p, '=')) {
          inner_name_ = detail::to_string(beg, p);
          param_state_ = param_state::value;
          return detail::len(beg, p) + 1;
        }

        if (detail::byte_is(*p, ' ')) {
          // When there are at least one characters in '&' and ' ', trated
          // them as parameter name.
          if (beg != p) {
            inner_name_ = detail::to_string(beg, p);
            message_->params.emplace(inner_name_, "");
          }
          param_state_ = param_state::completed;
          return detail::len(beg, p);
        }

        if (!detail::is_uri_char(*p)) {
          return net::unexpected(make_error_code(error::bad_params));
        }
      }

      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @details According to RFC 9110:
     *               parameter-value = ( token / quoted-string )
     *               OWS             = *( SP / HTAB )
     *                               ; optional whitespace
    */
    size_expected parse_param_value(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        if (detail::byte_is(*p, '&')) {
          message_->params.emplace(inner_name_, detail::to_string(beg, p));
          param_state_ = param_state::name;
          return detail::len(beg, p) + 1;
        }

        if (detail::byte_is(*p, ' ')) {
          message_->params.emplace(inner_name_, detail::to_string(beg, p));
          param_state_ = param_state::completed;
          return detail::len(beg, p);
        }

        if (!detail::is_uri_char(*p)) {
          return net::unexpected(make_error_code(error::bad_params));
        }
      }

      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @brief Parse http request parameters.
     * @details According to RFC 9110:
     *               parameters      = *( OWS ";" OWS [ parameter ] )
     *               parameter       = parameter-name "=" parameter-value
    */
    size_expected parse_params(cpointer beg, cpointer end) noexcept {
      static_assert(http1_request_concept<Message>);
      size_expected result = 0;
      std::size_t total_parsed_len = 0;
      cpointer p = beg;

      while (true) {
        switch (param_state_) {
        case param_state::name: {
          result = parse_param_name(p, end);
          break;
        }
        case param_state::value: {
          result = parse_param_value(p, end);
          break;
        }
        case param_state::completed: {
          inner_name_.clear();
          uri_state_ = uri_state::completed;
          return detail::len(beg, p);
        }
        }
        if (!result) {
          return result;
        }
        p += *result;
      }
    }

    /*
     * @details Parse whitespaces before http version. Multiply whitespaces are
     * allowd between uri and http version.
    */
    size_expected parse_spaces_before_version(cpointer beg, cpointer end) noexcept {
      const std::byte* p = detail::trim_front(beg, end);
      if (p == end) [[unlikely]] {
        return net::unexpected(make_error_code(error::need_more));
      }

      request_line_state_ = request_line_state::version;
      return detail::len(beg, p);
    }

    /*
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
    size_expected parse_request_http_version(cpointer beg, cpointer end) noexcept {
      constexpr std::size_t version_length = 10;
      if (detail::len(beg, end) < version_length) {
        return net::unexpected(make_error_code(error::need_more));
      }
      if (
        beg[0] != std::byte{'H'} ||  //
        beg[1] != std::byte{'T'} ||  //
        beg[2] != std::byte{'T'} ||  //
        beg[3] != std::byte{'P'} ||  //
        beg[4] != std::byte{'/'} ||  //
        !detail::is_digit(beg[5]) || //
        beg[6] != std::byte{'.'} ||  //
        !detail::is_digit(beg[7]) || //
        beg[8] != std::byte{'\r'} || //
        beg[9] != std::byte{'\n'}) {
        return net::unexpected(make_error_code(error::bad_version));
      }

      auto version = to_http_version(std::to_integer<int>(beg[5]), std::to_integer<int>(beg[7]));
      message_->version = version;
      request_line_state_ == request_line_state::completed;
      return version_length;
    }

    /*
     * @details According to RFC 9112:
     *                   status-line = version SP status-code SP reason-phrase
     * Parse http status line. The first line of a response message is
     * the status-line, consisting of the protocol version, a space (SP), the
     * status code, and another space and ending with an OPTIONAL textual phrase
     * describing the status code.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-status-line
    */
    size_expected parse_status_line(cpointer beg, cpointer end) noexcept {
      size_expected result = 0;
      std::size_t total_parsed_len = 0;
      cpointer p = beg;
      while (true) {
        switch (status_line_state_) {
        case status_line_state::version: {
          result = parse_rsponse_http_version(p, end);
          break;
        }
        case status_line_state::status_code: {
          result = parse_status_code(p, end);
          break;
        }
        case status_line_state::reason: {
          result = parse_reason(p, end);
          break;
        }
        case status_line_state::completed: {
          state_ = http1_parse_state::expecting_newline;
          return detail::len(beg, p);
        }
        }
        if (!result) {
          return result;
        }
        p += *result;
      }
    }

    /*
     * @details Like @ref parse_request_http_version, but doesn't need CRLF. Instead,
     * this function needs SP to indicate version is received completed.
    */
    size_expected parse_rsponse_http_version(cpointer beg, cpointer end) noexcept {
      // HTTP/1.1 + SP
      constexpr std::size_t version_length = 9;
      if (detail::len(beg, end) < version_length) {
        return net::unexpected(make_error_code(error::need_more));
      }
      if (
        beg[0] != std::byte{'H'} ||  //
        beg[1] != std::byte{'T'} ||  //
        beg[2] != std::byte{'T'} ||  //
        beg[3] != std::byte{'P'} ||  //
        beg[4] != std::byte{'/'} ||  //
        !detail::is_digit(beg[5]) || //
        beg[6] != std::byte{'.'} ||  //
        !detail::is_digit(beg[7]) || //
        beg[8] != std::byte{' '}) {
        return net::unexpected(make_error_code(error::bad_version));
      }

      message_->version = to_http_version(
        std::to_integer<int>(beg[5]), std::to_integer<int>(beg[7]));
      status_line_state_ = status_line_state::status_code;
      return version_length;
    }

    /*
     * @details According to RFC 9112:
     *                   status-code = 3DIGIT
     * The status-code element is a 3-digit integer code describing the result of
     * the server's attempt to understand and satisfy the client's corresponding
     * request.
    */
    size_expected parse_status_code(cpointer beg, cpointer /*end*/) noexcept {
      // 3DIGIT + SP
      constexpr std::size_t status_length = 4;
      if (status_length < 4) {
        return net::unexpected(make_error_code(error::need_more));
      }
      if (!detail::byte_is(beg[3], ' ')) {
        return net::unexpected(make_error_code(error::bad_status));
      }
      message_->status_code = to_http_status_code(detail::to_string_view(beg, 3));
      if (message_->status_code == http_status_code::unknown) {
        return net::unexpected(make_error_code(error::unknown_status));
      }

      status_line_state_ = status_line_state::reason;
      return status_length;
    }

    /*
     * @details According to RFC 9112:
     *               reason-phrase  = 1*( HTAB / SP / VCHAR)
     *   The reason-phrase element exists for the sole purpose of providing a
     * textual description associated with the numeric status code, mostly out of
     * deference to earlier Internet application protocols that were more
     * frequently used with interactive text clients.
    */
    size_expected parse_reason(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        if (!detail::byte_is(*p, '\r')) {
          continue;
        }
        if (!detail::byte_is(*(p + 1), '\n')) {
          return net::unexpected(make_error_code(error::bad_line_ending));
        }

        message_->reason = detail::to_string(beg, p);
        state_ = http1_parse_state::expecting_newline;
        return detail::len(beg, p) + 2;
      }
      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @details Parse a new line. If the new line just is "\r\n", it means that
     * all the headers have been parsed and the body can be parsed. Otherwise,
     * the line is still a header line.
    */
    size_expected parse_expecting_new_line(cpointer beg, cpointer end) noexcept {
      constexpr std::size_t line_ending_length = 2;
      if (detail::len(beg, end) < line_ending_length) {
        return net::unexpected(make_error_code(error::need_more));
      }
      if (detail::compare_2_char(beg, '\r', '\n')) {
        state_ = http1_parse_state::body;
      } else {
        state_ = http1_parse_state::header;
      }
      return line_ending_length;
    }

    /*
     * @details: According to RFC 9110:
     *                        header-name    = tokens
     * Header names are case-insensitive. And the header name is not allowed to
     * be empty. Any leading whitespace is aslo not allowed. Note that the
     * header name maybe repeated. All duplicated header names are saved as-is
     * and in order.
     * @see https://datatracker.ietf.org/doc/html/rfc9110#name-field-names
    */
    size_expected parse_header_name(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        if (detail::byte_is(*p, ':')) {
          if (p == beg) [[unlikely]] {
            return net::unexpected(make_error_code(error::empty_header_name));
          }

          inner_name_ = detail::to_string(beg, p);
          header_state_ = header_state::spaces_before_value;
          return detail::len(beg, p) + 1;
        }
        if (!detail::is_token(*p)) {
          return net::unexpected(make_error_code(error::bad_header_name));
        }
      }
      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @details According to RFC 9110:
     *                field-value   = *field-content
     *                field-content = field-vchar
     *                                [ 1*( SP / HTAB / field-vchar ) field-vchar]
     *                field-vchar   = VCHAR / obs-text
     *                VCHAR         = any visible US-ASCII character
     *                obs-text      = %x80-FF
     * HTTP header value consist of a sequence of characters in a format
     * defined by the field's grammar. A header value does not include leading
     * or trailing whitespace. And it's not allowed to be empty.
     * @see https://datatracker.ietf.org/doc/html/rfc9110#name-field-values
    */
    size_expected parse_header_value(cpointer beg, cpointer end) noexcept {
      for (const std::byte* p = beg; p < end; ++p) {
        // Focus only on '\r'.
        if (detail::byte_is(*p, '\r')) {
          continue;
        }

        // Skip the tail whitespaces.
        const std::byte* whitespace = p - 1;
        while (detail::byte_is(*whitespace, ' ')) {
          --whitespace;
        }

        // Empty header value.
        if (whitespace == beg - 1) {
          return net::unexpected(make_error_code(error::empty_header_name));
        }

        message_->headers.emplace(inner_name_, detail::to_string(beg, whitespace + 1));
        inner_name_.clear();
        header_state_ = header_state::header_line_ending;
        return detail::len(beg, p);
      }
      return net::unexpected(make_error_code(error::need_more));
    }

    /*
     * @details Parse spaces between header name and header value.
    */
    size_expected parse_spaces_before_header_value(cpointer beg, cpointer end) noexcept {
      const std::byte* p = detail::trim_front(beg, end);
      if (p == end) {
        return net::unexpected(make_error_code(error::need_more));
      }
      header_state_ = header_state::value;
      return detail::len(beg, p);
    }

    /*
     * @brief Parse http header line ending.
     * @details According to RFC 9110:
     *                       header line ending = "\r\n"
    */
    size_expected parse_header_line_ending(cpointer beg, cpointer end) {
      constexpr std::size_t line_ending_length = 2;
      if (detail::len(beg, end) < line_ending_length) {
        return net::unexpected(make_error_code(error::need_more));
      }
      if (!detail::compare_2_char(beg, '\r', '\n')) {
        return net::unexpected(make_error_code(error::bad_line_ending));
      }
      header_state_ = header_state::completed;
      return line_ending_length;
    }

    void_expected parse_special_headers() noexcept {
      return parse_header_host()
        .and_then([this] { return parse_header_content_length(); })
        .and_then([this] { return parse_header_connection(); });
    }

    void_expected parse_header_connection() noexcept {
      return {};
    }

    void_expected parse_header_host() noexcept {
      return {};
    }

    void_expected parse_header_content_length() noexcept {
      auto range = message_->heades.equal_range(http_header_content_length);
      auto header_cnt = std::distance(range.first, range.second);
      if (header_cnt > 1) {
        return net::unexpected(make_error_code(error::multiple_content_length));
      }
      if (header_cnt == 0) {
        message_->content_length = 0;
        return {};
      }

      std::size_t len = 0;
      auto [_, res] = std::from_chars(
        range.first->data(), //
        range.first->data() + range.first->size(),
        len);
      if (res != std::errc()) {
        return net::unexpected(make_error_code(error::bad_content_length));
      }
      message_->content_length = len;
    }

    /*
     * @details Parse http headers. HTTP headers let the client and the server
     * pass additional information with an HTTP request or response. An HTTP
     * header consists of it's case-insensitive name followed by a colon (:), then
     * by its value. Whitespace before the value is ignored. Note that the header
     * name maybe repeated.
     * @see https://datatracker.ietf.org/doc/html/rfc9110#name-fields
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-field-syntax
    */
    size_expected parse_header(cpointer beg, cpointer end) noexcept {
      size_expected result = 0;
      cpointer p = beg;

      header_state_ = header_state::name;
      while (true) {
        switch (header_state_) {
        case header_state::name: {
          result = parse_header_name(p, end);
          break;
        }
        case header_state::spaces_before_value: {
          result = parse_spaces_before_header_value(p, end);
          break;
        }
        case header_state::value: {
          result = parse_header_value(p, end);
          break;
        }
        case header_state::header_line_ending: {
          result = parse_header_line_ending(p, end);
          break;
        }
        case header_state::completed: {
          state_ = http1_parse_state::expecting_newline;
          return p - beg;
        }
        }
        if (!result) {
          return result;
        }
        p += *result;
      }
    }

    /*
     * @details According to RFC 9110:
     *                  message-body = *OCTET
     *                         OCTET =  any 8-bit sequence of data
     *   The message body (if any) of an HTTP message is used to carry content
     *   for the request or response.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-message-body
     * @todo Currently this library only deal with Content-Length case.
    */
    size_expected parse_body(cpointer beg, cpointer end) noexcept {
      if (message_->content_length == 0) {
        state_ = http1_parse_state::completed;
        return 0;
      }
      if (detail::len(beg, end) < message_->content_length) {
        return net::unexpected(make_error_code(error::need_more));
      }
      if (detail::len(beg, end) > message_->content_length) {
        return net::unexpected(make_error_code(error::body_size_bigger_than_content_length));
      }

      message_->body = detail::to_string(beg, message_->content_length);
      state_ = http1_parse_state::completed;
      return message_->content_length;
    }

    // Parse states.
    http1_parse_state state_ = http1_parse_state::nothing_yet;
    request_line_state request_line_state_ = request_line_state::method;
    status_line_state status_line_state_ = status_line_state::version;
    uri_state uri_state_ = uri_state::initial;
    param_state param_state_ = param_state::name;
    header_state header_state_ = header_state::name;

    // Used by header name and parameter name. Name and value are parsed in two
    // functions, save name to be later used in value parse functions.
    std::string inner_name_;

    // The message to be filled.
    Message* message_ = nullptr;
  };

} // namespace net::http::http1
