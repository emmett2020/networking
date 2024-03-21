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

#include <strings.h>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <limits>
#include <span>
#include <string>
#include <system_error>

#include <fmt/core.h>

#include "http1/http1_common.h"
#include "http1/http1_concept.h"
#include "http1/http1_error.h"
#include "http1/http1_request.h"
#include "http1/http1_response.h"

// TODO: Refactor all documents. Refine some comments.
// TODO: Still need to implement and optimize this parser. Write more
// test case to make this parser robust. This is a long time work, we need to
// implement others first.
// TODO: Add some UTF-8 test cases. We should support theses encoding type: Latin1, UTF-8, ASCII,
//       especially UTF-8.
// TODO: support chunk
// TODO: how about std::ranges coding style?

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
    constexpr std::array<std::uint8_t, 256> token_table = {
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
    inline bool is_token(std::byte b) noexcept {
      return static_cast<bool>(token_table[std::to_integer<uint8_t>(b)]);
    }

    inline bool byte_is(std::byte b, char expect) noexcept {
      return b == static_cast<std::byte>(expect);
    }

    // In some platforms, char implements as signed char while other platforms implements unsigned char.
    // To be safely uses type char, we assume char values 0-127. But std::byte represents a octet, which values 0-255.
    // So to be safely use this function on all platforms, we must ensure that std::byte only uses 0-127 either.
    inline std::string_view
      string_view(const std::byte* const beg, const std::byte* const end) noexcept {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return {reinterpret_cast<const char*>(beg), reinterpret_cast<const char*>(end)};
    }

    inline std::string_view string_view(const std::byte* const beg, std::uint32_t size) noexcept {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return {reinterpret_cast<const char*>(beg), size};
    }

    inline std::string string(const std::byte* const beg, const std::byte* const end) noexcept {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return {reinterpret_cast<const char*>(beg), reinterpret_cast<const char*>(end)};
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
    inline bool is_path_char(std::byte b) noexcept {
      constexpr std::array<uint8_t, 256> table{
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
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // 240 - 255
      };
      return static_cast<bool>(table[std::to_integer<uint8_t>(b)]);
    }

    /*
     * @brief A helper function to make string view use two pointers.
     * @param first The start address of the data.
     * @param last  The end address of the data. The data range is half-opened.
     * @attention To be consistent with parse_xxx functions, the data range of this
     *            function is also half-open: [first, last).
    */
    // inline std::string_view to_string(const std::byte* first, const std::byte* last) noexcept {
    // return std::basic_string_view<std::byte>{first, static_cast<std::size_t>(last - first)};
    // }

    inline bool is_alpha(std::byte b) {
      return (b | std::byte{0x20}) >= std::byte{'a'} && (b | std::byte{0x20}) <= std::byte{'z'};
    }

    inline bool is_digit(std::byte b) {
      return (b | std::byte{0x20}) >= std::byte{'0'} && (b | std::byte{0x20}) <= std::byte{'9'};
    }

    inline bool is_alnum(std::byte b) {
      return is_alpha(b) && is_digit(b);
    }

    /*
     * @brief Skip the whitespace from front.
     * @param first The start address of the data.
     * @param last  The end address of the data. The data range is half-opened.
     * @return The pointer which points to the first non-whitespace position or last
     *         which represents all charater between [first, last) is white space.
    */
    inline const std::byte* trim_front(const std::byte* first, const std::byte* last) noexcept {
      while (first < last && (*first == std::byte{' '} || *first == std::byte{'\t'})) {
        ++first;
      }
      return first;
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

    inline std::uint32_t len(const std::byte* beg, const std::byte* end) noexcept {
      return end - beg;
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

    inline http_method to_http_method(const std::byte* const beg, const std::byte* const end) {
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
          return http_method::delete_;
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

    inline std::uint16_t default_port(http_scheme scheme) noexcept {
      if (scheme == http_scheme::http) {
        return 80;
      }
      if (scheme == http_scheme::https) {
        return 443;
      }
      return 0;
    }

  } // namespace detail

  // Indicates current parser's state.
  enum class parse_state {
    nothing_yet,
    start_line,
    expecting_newline,
    header,
    body,
    completed
  };

  /*
   * Introduction:
   *   This parser is used for parse HTTP/1.0 and HTTP/1.1 message. According to
   * RFC9112, A HTTP/1.1 message consists of a start-line followed by a CRLF and
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
  template <http1_message Message>
  class message_parser {
    using error_code = std::error_code;

   public:
    explicit message_parser(Message* message) noexcept
      : message_(message) {
      name_.reserve(8192);
    }

    void reset(Message* message) noexcept {
      message_ = message;
      reset();
    }

    void reset() noexcept {
      state_ = parse_state::nothing_yet;
      header_state_ = header_state::name;
      uri_state_ = uri_state::initial;
      param_state_ = param_state::name;
      inner_parsed_len_ = 0;
      name_.clear();
    }

    [[nodiscard]] Message* message() const noexcept {
      return message_;
    }

    std::size_t parse(std::span<const std::byte> const_buffer, error_code& ec) {
      assert(
        !ec && "net::http1::message_parser::parse() failed. The parameter ec should be clear.");
      buffer buf{
        .beg = const_buffer.data(),
        .end = const_buffer.data() + const_buffer.size(),
        .parsed_len = 0,
      };
      while (ec == std::errc{}) {
        switch (state_) {
        case parse_state::nothing_yet:
        case parse_state::start_line: {
          if constexpr (http1_request<Message>) {
            parse_request_line(buf, ec);
          } else {
            parse_status_line(buf, ec);
          }
          break;
        }
        case parse_state::expecting_newline: {
          parse_expecting_new_line(buf, ec);
          break;
        }
        case parse_state::header: {
          parse_header(buf, ec);
          break;
        }
        case parse_state::body: {
          parse_body(buf, ec);
          break;
        }
        case parse_state::completed: {
          return buf.parsed_len;
        }
        }
      }
      // If fatal error occurs, return zero as parsed size.
      return ec == error::need_more ? buf.parsed_len : 0;
    }

    [[nodiscard]] parse_state State() const noexcept {
      return state_;
    }

   private:
    struct buffer {
      const std::byte* beg{nullptr};
      const std::byte* end{nullptr};
      std::size_t parsed_len{0};
    };

    enum class request_line_state {
      method,
      spaces_before_uri,
      uri,
      spaces_before_http_version,
      version,
    };

    enum class status_line_state {
      version,
      status_code,
      reason,
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
      header_line_ending
    };

    enum class param_state {
      name,
      value,
      completed
    };

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9112:
     *               request-line   = method SP request-target SP HTTP-version
     *   A request-line begins with a method token, followed by a single space
     * (SP), the request-target, and another single space (SP), and ends with the
     * protocol version.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-request-line
    */
    void parse_request_line(buffer& buf, error_code& ec) {
      while (ec == std::errc{}) {
        switch (request_line_state_) {
        case request_line_state::method: {
          parse_method(buf, ec);
          break;
        }
        case request_line_state::spaces_before_uri: {
          parse_spaces_before_uri(buf, ec);
          break;
        }
        case request_line_state::uri: {
          parse_uri(buf, ec);
          break;
        }
        case request_line_state::spaces_before_http_version: {
          parse_spaces_before_version(buf, ec);
          break;
        }
        case request_line_state::version: {
          parse_request_http_version(buf, ec);
          return;
        }
        }
      }
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9112:
     *                        method = token
     * The method token is case-sensitive. By convention, standardized methods are
     * defined in all-uppercase US-ASCII letters and not allowd to be empty. If no
     * error occurred, the method field of inner request will be filled and the
     * state_ of this parser will be changed from method to spaces_before_uri.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-method
    */
    void parse_method(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* method_beg = buf.beg + buf.parsed_len;
      for (const std::byte* p = method_beg; p < buf.end; ++p) {
        // Collect method string until met first non-method charater.
        if (detail::is_token(*p)) [[likely]] {
          continue;
        }

        // The first character after method string must be whitespace.
        if (*p != std::byte{' '}) {
          ec = error::bad_method;
          return;
        }

        // Empty method is not allowed.
        if (p == method_beg) {
          ec = error::empty_method;
          return;
        }

        message_->method = detail::to_http_method(method_beg, p);
        if (message_->method == http_method::unknown) {
          ec = error::bad_method;
          return;
        }

        buf.parsed_len += p - method_beg;
        request_line_state_ = request_line_state::spaces_before_uri;
        return;
      }
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details Parse white spaces before HTTP URI. Multiply whitespaces are
     * allowd between method and URI.
    */
    void parse_spaces_before_uri(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* spaces_beg = buf.beg + buf.parsed_len;
      const std::byte* p = detail::trim_front(spaces_beg, buf.end);
      if (p == buf.end) [[unlikely]] {
        ec = error::need_more;
        return;
      }
      buf.parsed_len += p - spaces_beg;
      request_line_state_ = request_line_state::uri;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_uri(buffer& buf, error_code& ec) noexcept {
      static_assert(http1_request<Message>);
      const std::byte* uri_beg = buf.beg + buf.parsed_len;
      if (uri_beg >= buf.end) {
        ec = error::need_more;
        return;
      }

      uri_state_ = uri_state::initial;
      while (ec == std::errc{}) {
        switch (uri_state_) {
        case uri_state::initial: {
          if (*uri_beg == std::byte{'/'}) {
            message_->port = 80;
            uri_state_ = uri_state::path;
          } else {
            uri_state_ = uri_state::scheme;
          }
          inner_parsed_len_ = 0;
          break;
        }
        case uri_state::scheme: {
          parse_scheme(buf, ec);
          break;
        }
        case uri_state::host: {
          parse_host(buf, ec);
          break;
        }
        case uri_state::port: {
          parse_port(buf, ec);
          break;
        }
        case uri_state::path: {
          parse_path(buf, ec);
          break;
        }
        case uri_state::params: {
          parse_params(buf, ec);
          break;
        }
        case uri_state::completed: {
          request_line_state_ = request_line_state::spaces_before_http_version;
          message_->uri = {uri_beg, uri_beg + inner_parsed_len_};
          buf.parsed_len += inner_parsed_len_;
          inner_parsed_len_ = 0;
          return;
        }
        }
      }
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9110:
     *                        scheme = "+" | "-" | "." | DIGIT | ALPHA
     * Parse http request scheme. The scheme are case-insensitive and normally
     * provided in lowercase. This library mainly parse two schemes: "http"
     * URI scheme and "https" URI scheme. If no error occurs, the inner request's
     * scheme field will be filled.
     * @see https://datatracker.ietf.org/doc/html/rfc9110#name-http-uri-scheme
    */
    void parse_scheme(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* uri_beg = buf.beg + buf.parsed_len;
      const std::byte* scheme_beg = uri_beg + inner_parsed_len_;
      for (const std::byte* p = scheme_beg; p < buf.end; ++p) {
        // Skip the scheme character to find first colon.
        if (detail::is_alnum(*p) || detail::one_of(*p, '+', '-', '.')) {
          continue;
        }

        // Need more data.
        if (detail::len(p, buf.end) < 2) {
          ec = error::need_more;
          return;
        }

        // The characters next to the first non-scheme character must be "://".
        if (detail::compare_3_char(p, ':', '/', '/')) {
          ec = error::bad_scheme;
          return;
        }

        uint32_t scheme_len = detail::len(scheme_beg, p);
        if (scheme_len >= 5 && detail::case_compare_http_char(scheme_beg)) {
          message_->scheme = http_scheme::https;
        } else if (scheme_len >= 4 && detail::case_compare_http_char(scheme_beg)) {
          message_->scheme = http_scheme::http;
        } else {
          message_->scheme = http_scheme::unknown;
        }

        inner_parsed_len_ = scheme_len + 3;
        uri_state_ = uri_state::host;
        return;
      }

      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_host(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* uri_beg = buf.beg + buf.parsed_len;
      const std::byte* host_beg = uri_beg + inner_parsed_len_;
      for (const auto* p = host_beg; p < buf.end; ++p) {
        // Collect host string until met first non-host charater.
        if (detail::is_alnum(*p) || detail::one_of(*p, '-', '.')) {
          continue;
        }

        // The charater after host must be follows.
        if (!detail::one_of(*p, ':', '/', '?', ' ')) {
          ec = error::bad_host;
          return;
        }

        // An empty host is not allowed at "http" scheme and "https" scheme.
        if (
          p == host_beg
          && (message_->scheme == http_scheme::http || message_->scheme == http_scheme::https)) {
          ec = error::bad_host;
          return;
        }

        // Fill host field of inner request.
        uint32_t host_len = detail::len(host_beg, p);
        message_->host = {host_beg, p};

        // Go to next state depends on the first charater after host identifier.
        switch (*p) {
        case std::byte{':'}:
          inner_parsed_len_ += host_len + 1;
          uri_state_ = uri_state::port;
          return;
        case std::byte{'/'}:
          message_->port = detail::default_port(message_->scheme);
          inner_parsed_len_ += host_len;
          uri_state_ = uri_state::path;
          return;
        case std::byte{'?'}:
          message_->port = detail::default_port(message_->scheme);
          inner_parsed_len_ += host_len + 1;
          uri_state_ = uri_state::params;
          return;
        case std::byte{' '}:
          message_->port = detail::default_port(message_->scheme);
          inner_parsed_len_ += host_len;
          uri_state_ = uri_state::completed;
          return;
        }
      }
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_port(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      uint16_t acc = 0;
      uint16_t cur = 0;

      const std::byte* uri_beg = buf.beg + buf.parsed_len;
      const std::byte* port_beg = uri_beg + inner_parsed_len_;

      for (const std::byte* p = port_beg; p < buf.end; ++p) {
        // Calculate the port value and save it in acc variable.
        if (detail::is_digit(*p)) [[likely]] {
          cur = std::to_integer<int>(*p);
          if (acc * 10 + cur > std::numeric_limits<uint16_t>::max()) {
            ec = error::bad_port;
            return;
          }
          acc = acc * 10 + cur;
          continue;
        }

        // The first character next to port string should be one of follows.
        if (!detail::one_of(*p, '/', '?', ' ')) {
          ec = error::bad_port;
          return;
        }

        // Port is zero, so use default port value based on scheme.
        if (acc == 0) {
          acc = detail::default_port(message_->scheme);
        }
        message_->port = acc;
        uint32_t port_string_len = detail::len(port_beg, p);
        switch (*p) {
        case std::byte{'/'}: {
          inner_parsed_len_ += port_string_len;
          uri_state_ = uri_state::path;
          return;
        }
        case std::byte{'?'}: {
          inner_parsed_len_ += port_string_len + 1;
          uri_state_ = uri_state::params;
          return;
        }
        case std::byte{' '}: {
          inner_parsed_len_ += port_string_len;
          uri_state_ = uri_state::completed;
          return;
        }
        } // switch
      }   // for
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9110:
     *                          path = token
     * Parse http request path. Http request path is defined as the path and
     * filename in a request. It does not include scheme, host, port or query
     * string. If no error occurs, the inner request's path field will be filled.
    */
    void parse_path(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* const uri_beg = buf.beg + buf.parsed_len;
      const std::byte* path_beg = uri_beg + inner_parsed_len_;
      for (const std::byte* p = path_beg; p < buf.end; ++p) {
        if (detail::byte_is(*p, '?')) {
          inner_parsed_len_ += p - path_beg + 1;
          message_->path = {path_beg, p};
          uri_state_ = uri_state::params;
          return;
        }

        if (detail::byte_is(*p, ' ')) {
          inner_parsed_len_ += p - path_beg;
          message_->path = {path_beg, p};
          uri_state_ = uri_state::completed;
          return;
        }
        if (!detail::is_path_char(*p)) {
          ec = error::bad_path;
          return;
        }
      } // for
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9110:
     *               parameter-name  = token
     * The parameter name is allowed to be empty only when explicitly set the
     * equal mark. In all other situations, an empty parameter name is
     * forbiddened. Note that there isn't a clear specification illustrate what to
     * do when there is a repeated key, we just use the last-occur policy now.
    */
    void parse_param_name(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* uri_beg = buf.beg + buf.parsed_len;
      const std::byte* name_beg = uri_beg + inner_parsed_len_;
      for (const std::byte* p = name_beg; p < buf.end; ++p) {
        if (detail::byte_is(*p, '&')) {
          // Skip the "?&" or continuous '&' , which represents empty
          // parameter name and parameter value.
          if (p == name_beg || p - name_beg == 1) {
            inner_parsed_len_ += 1;
            return;
          }

          // When all characters in two ampersand don't contain =, all
          // characters in ampersand (except ampersand) are considered
          // parameter names.
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          name_ = {reinterpret_cast<const char*>(name_beg), reinterpret_cast<const char*>(p)};
          message_->params[name_] = "";
          inner_parsed_len_ += p - name_beg + 1;
          return;
        }

        if (detail::byte_is(*p, '=')) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          name_ = {reinterpret_cast<const char*>(name_beg), reinterpret_cast<const char*>(p)};
          inner_parsed_len_ += p - name_beg + 1;
          param_state_ = param_state::value;
          return;
        }

        if (detail::byte_is(*p, ' ')) {
          // When there are at least one characters in '&' and ' ', trated
          // them as parameter name.
          if (name_beg != p) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            name_ = {reinterpret_cast<const char*>(name_beg), reinterpret_cast<const char*>(p)};
            message_->params[name_] = "";
          }
          inner_parsed_len_ += p - name_beg;
          param_state_ = param_state::completed;
          return;
        }

        if (!detail::is_path_char(*p)) {
          ec = error::bad_params;
          return;
        }
      }
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9110:
     *               parameter-value = ( token / quoted-string )
     *               OWS             = *( SP / HTAB )
     *                               ; optional whitespace
    */
    void parse_param_value(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* uri_beg = buf.beg + buf.parsed_len;
      const std::byte* value_beg = uri_beg + inner_parsed_len_;
      for (const std::byte* p = value_beg; p < buf.end; ++p) {
        if (detail::byte_is(*p, '&')) {
          message_->params[name_] = {value_beg, p};
          inner_parsed_len_ += p - value_beg + 1;
          param_state_ = param_state::name;
          return;
        }
        if (detail::byte_is(*p, ' ')) {
          message_->params[name_] = {value_beg, p};
          inner_parsed_len_ += p - value_beg;
          param_state_ = param_state::completed;
          return;
        }
        if (!detail::is_path_char(*p)) {
          ec = error::bad_params;
          return;
        }
      }
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_params(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      param_state_ = param_state::name;
      while (ec == std::errc{}) {
        switch (param_state_) {
        case param_state::name: {
          parse_param_name(buf, ec);
          break;
        }
        case param_state::value: {
          parse_param_value(buf, ec);
          break;
        }
        case param_state::completed: {
          name_.clear();
          uri_state_ = uri_state::completed;
          return;
        }
        } // switch
      }   // while
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details Parse white spaces before http version. Multiply whitespaces are
     * allowd between uri and http version.
    */
    void parse_spaces_before_version(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* spaces_beg = buf.beg + buf.parsed_len;
      const std::byte* p = detail::trim_front(spaces_beg, buf.end);
      if (p == buf.end) [[unlikely]] {
        ec = error::need_more;
        return;
      }
      buf.parsed_len += p - spaces_beg;
      request_line_state_ = request_line_state::version;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_request_http_version(buffer& buf, error_code& ec) {
      static_assert(http1_request<Message>);
      const std::byte* version_beg = buf.beg + buf.parsed_len;
      if (buf.end - version_beg < 10) {
        ec = error::need_more;
        return;
      }
      if (
        version_beg[0] != std::byte{'H'} ||  //
        version_beg[1] != std::byte{'T'} ||  //
        version_beg[2] != std::byte{'T'} ||  //
        version_beg[3] != std::byte{'P'} ||  //
        version_beg[4] != std::byte{'/'} ||  //
        !detail::is_digit(version_beg[5]) || //
        version_beg[6] != std::byte{'.'} ||  //
        !detail::is_digit(version_beg[7]) || //
        version_beg[8] != std::byte{'\r'} || //
        version_beg[9] != std::byte{'\n'}) {
        ec = error::bad_version;
        return;
      }

      message_->version = to_http_version(
        std::to_integer<int>(version_beg[5]), std::to_integer<int>(version_beg[7]));
      buf.parsed_len += 10;
      state_ = parse_state::expecting_newline;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9112:
     *                   status-line = version SP status-code SP reason-phrase
     * Parse http status line. The first line of a response message is
     * the status-line, consisting of the protocol version, a space (SP), the
     * status code, and another space and ending with an OPTIONAL textual phrase
     * describing the status code.
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-status-line
    */
    void parse_status_line(buffer& buf, error_code& ec) {
      while (ec == std::errc{}) {
        switch (status_line_state_) {
        case status_line_state::version: {
          parse_rsponse_http_version(buf, ec);
          break;
        }
        case status_line_state::status_code: {
          parse_status_code(buf, ec);
          break;
        }
        case status_line_state::reason: {
          parse_reason(buf, ec);
          return;
        }
        }
      }
    }

    /*
     * @param arg Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details Like @ref parse_request_http_version, but doesn't need CRLF. Instead,
     * this function needs SP to indicate version is received completed.
    */
    void parse_rsponse_http_version(buffer& buf, error_code& ec) {
      static_assert(http1_response<Message>);
      const std::byte* version_beg = buf.beg + buf.parsed_len;
      // HTTP-name + SP
      if (detail::len(version_beg, buf.end) < 9) {
        ec = error::need_more;
        return;
      }
      if (
        version_beg[0] != std::byte{'H'} ||  //
        version_beg[1] != std::byte{'T'} ||  //
        version_beg[2] != std::byte{'T'} ||  //
        version_beg[3] != std::byte{'P'} ||  //
        version_beg[4] != std::byte{'/'} ||  //
        !detail::is_digit(version_beg[5]) || //
        version_beg[6] != std::byte{'.'} ||  //
        !detail::is_digit(version_beg[7]) || //
        version_beg[8] != std::byte{' '}) {
        ec = error::bad_version;
        return;
      }

      message_->version = to_http_version(
        std::to_integer<int>(version_beg[5]), std::to_integer<int>(version_beg[7]));
      buf.parsed_len += 9;
      status_line_state_ = status_line_state::status_code;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9112:
     *                   status-code = 3DIGIT
     * The status-code element is a 3-digit integer code describing the result of
     * the server's attempt to understand and satisfy the client's corresponding
     * request.
    */
    void parse_status_code(buffer& buf, error_code& ec) {
      static_assert(http1_response<Message>);
      const std::byte* status_code_beg = buf.beg + buf.parsed_len;
      // 3DIGIT + SP
      if (detail::len(status_code_beg, buf.end) < 4) {
        ec = error::need_more;
        return;
      }
      if (!detail::byte_is(status_code_beg[3], ' ')) {
        ec = error::bad_status;
        return;
      }
      message_->status_code = to_http_status_code(detail::string_view(status_code_beg, 3));
      if (message_->status_code == http_status_code::unknown) {
        ec = error::bad_status;
        return;
      }
      buf.parsed_len += 4;
      status_line_state_ = status_line_state::reason;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9112:
     *               reason-phrase  = 1*( HTAB / SP / VCHAR)
     *
     *   The reason-phrase element exists for the sole purpose of providing a
     * textual description associated with the numeric status code, mostly out of
     * deference to earlier Internet application protocols that were more
     * frequently used with interactive text clients.
    */
    void parse_reason(buffer& buf, error_code& ec) {
      static_assert(http1_response<Message>);
      const std::byte* reason_beg = buf.beg + buf.parsed_len;
      for (const std::byte* p = reason_beg; p < buf.end; ++p) {
        if (!detail::byte_is(*p, '\r')) {
          continue;
        }

        if (!detail::byte_is(*(p + 1), '\n')) {
          ec = error::bad_line_ending;
          return;
        }
        message_->reason = {reason_beg, p};

        // Skip the "\r\n"
        buf.parsed_len += detail::len(reason_beg, p) + 2;
        state_ = parse_state::expecting_newline;
        return;
      }
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details Parse a new line. If the line is "\r\n", it means that all the
     *          headers have been parsed and the body can be parsed. Otherwise,
     *          the line is still a header line.
    */
    void parse_expecting_new_line(buffer& buf, error_code& ec) {
      const std::byte* line_beg = buf.beg + buf.parsed_len;
      if (detail::len(line_beg, buf.end) < 2) {
        ec = error::need_more;
        return;
      }
      if (detail::compare_2_char(line_beg, '\r', '\n')) {
        state_ = parse_state::body;
        buf.parsed_len += 2;
        return;
      }
      state_ = parse_state::header;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_header_name(buffer& buf, error_code& ec) {
      const std::byte* name_beg = buf.beg + buf.parsed_len;
      for (const std::byte* p = name_beg; p < buf.end; ++p) {
        if (detail::byte_is(*p, ':')) {
          if (p == name_beg) [[unlikely]] {
            ec = error::empty_header_name;
            return;
          }

          // Header name is case-insensitive. Save header name in lowercase.
          if (detail::len(name_beg, p) > name_.size()) {
            name_.resize(p - name_beg);
          }
          // BUG: implement this
          // std::transform(name_beg, p, name_.begin(), tolower);

          inner_parsed_len_ += p - name_beg + 1;
          header_state_ = header_state::spaces_before_value;
          return;
        }
        if (!detail::is_token(*p)) {
          ec = error::bad_header_name;
          return;
        }
      }
      ec = error::need_more;
    }

    /*
     * @brief When the header name is repeated, some header names can simply use
     * the last appeared value, while others can combine all the values into a
     * list as the final value. This function returns whether the given parameter
     * belongs to the second type of header.
    */
    static bool need_concat_header_value(std::string_view header_name) noexcept {
      return header_name == "accept-encoding";
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_header_value(buffer& buf, error_code& ec) {
      const std::byte* value_beg = buf.beg + buf.parsed_len + inner_parsed_len_;
      for (const std::byte* p = value_beg; p < buf.end; ++p) {
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
        if (whitespace == value_beg - 1) {
          ec = error::empty_header_value;
          return;
        }

        std::string header_value = detail::string(value_beg, whitespace + 1);

        // Concat all header values in a list.
        if (need_concat_header_value(name_) && message_->headers.contains(name_)) {
          message_->headers[name_] += "," + header_value;
        } else {
          message_->headers[name_] = header_value;
        }
        inner_parsed_len_ += p - value_beg;
        header_state_ = header_state::header_line_ending;
        return;
      }
      ec = error::need_more;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details Parse spaces between header name and header value.
    */
    void parse_spaces_before_header_value(buffer& buf, error_code& ec) {
      const std::byte* spaces_beg = buf.beg + buf.parsed_len + inner_parsed_len_;
      const std::byte* p = detail::trim_front(spaces_beg, buf.end);
      if (p == buf.end) {
        ec = error::need_more;
        return;
      }
      inner_parsed_len_ += p - spaces_beg;
      header_state_ = header_state::value;
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details According to RFC 9110:
     *            header line ending = "\r\n"
     * Parse http header line ending.
    */
    void parse_header_line_ending(buffer& buf, error_code& ec) {
      const std::byte* line_ending_beg = buf.beg + buf.parsed_len + inner_parsed_len_;
      if (detail::len(line_ending_beg, buf.end) < 2) {
        ec = error::need_more;
        return;
      }
      if (!detail::compare_2_char(line_ending_beg, '\r', '\n')) {
        ec = error::bad_line_ending;
        return;
      }

      // After a completed header line successfully parsed, parse some special
      // headers.
      parse_special_headers(name_, ec);
      if (ec) {
        return;
      }
      buf.parsed_len += inner_parsed_len_ + 2;
      name_.clear();
      inner_parsed_len_ = 0;
      header_state_ = header_state::name;
      state_ = parse_state::expecting_newline;
    }

    // Note that header name must be lowecase.
    void parse_special_headers(const std::string& header_name, error_code& ec) {
      if (header_name == "content-length") {
        return parse_header_content_length(ec);
      }

      if (header_name == "connection") {
        return parse_header_connection(ec);
      }
    }

    void parse_header_connection(error_code& ec) {
    }

    void parse_header_content_length(error_code& ec) {
      if (!message_->headers.contains("content-length")) {
        message_->content_length = 0;
      }

      std::string_view length_string = message_->headers["content-length"];
      std::size_t length{0};
      auto [_, res] = std::from_chars(length_string.begin(), length_string.end(), length);
      if (res == std::errc()) {
        message_->content_length = length;
      } else {
        ec = error::bad_content_length;
      }
    }

    /*
     * @param buf Records information about the currently parsed buffer.
     * @param ec The error code which holds detailed failure reason.
     * @details Parse http headers. HTTP headers let the client and the server
     * pass additional information with an HTTP request or response. An HTTP
     * header consists of it's case-insensitive name followed by a colon (:), then
     * by its value. Whitespace before the value is ignored. Note that the header
     * name maybe repeated.
     * @see https://datatracker.ietf.org/doc/html/rfc9110#name-fields
     * @see https://datatracker.ietf.org/doc/html/rfc9112#name-field-syntax
    */
    void parse_header(buffer& buf, error_code& ec) {
      inner_parsed_len_ = 0;
      header_state_ = header_state::name;
      while (ec == std::errc{}) {
        switch (header_state_) {
        case header_state::name: {
          parse_header_name(buf, ec);
          break;
        }
        case header_state::spaces_before_value: {
          parse_spaces_before_header_value(buf, ec);
          break;
        }
        case header_state::value: {
          parse_header_value(buf, ec);
          break;
        }
        case header_state::header_line_ending: {
          parse_header_line_ending(buf, ec);
          return;
        }
        } // switch
      }   // while
    }

    /*
     * @param buf Records information about the currently parsed buffer.
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
    void parse_body(buffer& buf, error_code& ec) {
      if (message_->content_length == 0) {
        state_ = parse_state::completed;
        return;
      }
      const std::byte* body_beg = buf.beg + buf.parsed_len;
      if (detail::len(body_beg, buf.end) < message_->content_length) {
        ec = error::need_more;
        return;
      }
      message_->body.reserve(message_->content_length);
      message_->body = {body_beg, body_beg + message_->content_length};
      buf.parsed_len += message_->content_length;
      state_ = parse_state::completed;
    }

    // States.
    // TODO: Use two bytes to represent all these states to save 4Byte memory one parser.
    // But we should reserch whether this approach will affect time profiency.
    parse_state state_ = parse_state::nothing_yet;
    request_line_state request_line_state_ = request_line_state::method;
    status_line_state status_line_state_ = status_line_state::version;
    uri_state uri_state_ = uri_state::initial;
    param_state param_state_ = param_state::name;
    header_state header_state_ = header_state::name;

    // Records the length of the buffer currently parsed, for URI and headers
    // only. Because both the URI and header contain multiple subparts, different
    // subparts are parsed by different parsing functions. When parsing a subpart,
    // we cannot update the parsed_len of the Argument directly, because we do
    // not know whether the URI or header can be successfully parsed in the end.
    // However, the subsequent function needs to know the location of the buffer
    // that the previous function  parsed, which is recorded by this variable.
    uint32_t inner_parsed_len_{0};


    // TODO: Do we really need this member?
    // Record the parameter name or header name.
    std::string name_;

    // Inner request. The request pointer must be made available during parser
    // parsing.
    // TODO: whether to use unique_ptr?
    Message* message_{nullptr};
  };

  // TODO: directions are confused, should we make another name to distinct these two requests?
  // using client_request_parser = message_parser<client_request>;
  // using server_request_parser
  // using server_response_parser = message_parser<response>;

} // namespace net::http1
