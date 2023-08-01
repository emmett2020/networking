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

#include "http/http_parser.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iterator>
#include <limits>
#include "http/http_error.h"

namespace net::http {

const char* BasicParser::TrimFront(const char* beg, const char* end) noexcept {
  while (beg != end && (*beg == ' ' || *beg == '\t')) {
    ++beg;
  }
  return beg;
}

const char* BasicParser::TrimTail(const char* beg, const char* end) noexcept {
  while (beg != end && (*end == ' ' || *end == '\t')) {
    --end;
  }
  return end;
}

bool BasicParser::IsPathChar(char input) noexcept {
  static constexpr std::array<uint8_t, 256> kTable{
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
  return static_cast<bool>(kTable[static_cast<uint8_t>(input)]);
}

bool BasicParser::ConvertToHexChar(uint8_t& result, char input) noexcept {
  static constexpr std::array<int8_t, 256> kTable{
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //   0
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  16
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  32
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1,  //  48
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  64
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  80
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  96
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 112
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 128
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 144
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 160
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 176
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 192
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 208
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 224
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1   // 240
  };
  result = static_cast<uint8_t>(kTable[static_cast<uint8_t>(input)]);
  return result != static_cast<uint8_t>(-1);
}

inline bool IsTokenChar(char c) noexcept {
  /*
      tchar = "!" | "#" | "$" | "%" | "&" |
              "'" | "*" | "+" | "-" | "." |
              "^" | "_" | "`" | "|" | "~" |
              DIGIT | ALPHA
  */
  static constexpr std::array<uint8_t, 256> kTable = {
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
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // 240
  };
  // TODO(xiaoming): uint8_t is 256, kTable is 240, should we fill it to 256?
  return static_cast<bool>(kTable[static_cast<uint8_t>(c)]);
}

// std::tuple<const char*, bool> BasicParser::FindFast(
//     const char* buf, const char* buf_end, const char* ranges,
//     size_t ranges_size) noexcept {
//   return {buf, false};
// }

const char* BasicParser::FindEol(const char* beg, const char* end) noexcept {
  while (beg != end) {
    if (*beg == '\r') {
      ++beg;
      if (beg == end) {
        return nullptr;
      }
      if (*beg != '\n') {
        return nullptr;
      }
      return ++beg;
    }
    ++beg;
  }
  return nullptr;
}

// TODO(xiaoming): ? why use string_view? unified with const char*
bool BasicParser::ParseDec(std::string_view s, uint64_t& result) noexcept {
  const char* it = s.data();
  const char* last = it + s.size();
  if (it == last) {
    return false;
  }
  uint64_t acc = 0;
  while (it != last) {
    if (!std::isdigit(*it) || acc > std::numeric_limits<uint64_t>::max() / 10) {
      return false;
    }
    acc *= 10;
    uint8_t cur = *it - '0';
    if (acc + cur > std::numeric_limits<uint64_t>::max()) {
      return false;
    }
    acc += cur;
  }
  result = acc;
  return true;
}

// TODO(xiaoming): so ugly
bool BasicParser::ParseHex(const char*& it, uint64_t& result) noexcept {
  uint8_t cur{0};
  if (!ConvertToHexChar(cur, *it)) {
    return false;
  }
  uint64_t acc = 0;
  do {
    if (acc > std::numeric_limits<uint64_t>::max() / 16) {
      return false;
    }
    acc *= 16;
    if (acc + cur > std::numeric_limits<uint64_t>::max()) {
      return false;
    }
    acc += cur;
  } while (ConvertToHexChar(cur, *++it));

  result = acc;
  return true;
}

// TODO(ming): doing what?
const char* BasicParser::FindEom(const char* beg, const char* end) noexcept {
  for (;;) {
    if (beg + 4 > end) {
      return nullptr;
    }
    if (beg[3] != '\n') {
      if (beg[3] == '\r') {
        ++beg;
      } else {
        beg += 4;
      }
    } else if (beg[2] != '\r') {
      beg += 4;
    } else if (beg[1] != '\n') {
      beg += 2;
    } else if (beg[0] != '\r') {
      beg += 2;
    } else {
      return beg + 4;
    }
  }
}

// TODO(xiaoming): 所有接口都改为pointer+State, 不要用这么传参的方式。
const char* BasicParser::ParseTokenToEol(const char* beg, const char* end,
                                         const char*& token_end,
                                         std::error_code& ec) noexcept {
  while (true) {
    if (beg >= end) {
      ec = Error::kNeedMore;
      return beg;
    }
    if (!std::isprint(*beg)) [[unlikely]] {
      if ((static_cast<uint8_t>(*beg) < 32 && *beg != 9)  //
          || *beg == 127) [[likely]] {
        break;
      }
    }
    ++beg;
  }

  if (*beg == '\r') [[likely]] {
    ++beg;
    if (beg >= end) {
      ec = Error::kNeedMore;
      return end;
    }
    if (*beg++ != '\n') {
      ec = Error::kBadLineEnding;
      return end;
    }
    token_end = beg - 2;
    return beg;
  } else {
    return nullptr;
  }
}

bool BasicParser::ParseCrlf(const char*& it) noexcept {
  if (it[0] != '\r' || it[1] != '\n') {
    return false;
  }
  it += 2;
  return true;
}

void BasicParser::ParseMethod(const char*& beg, const char* last,
                              std::string_view& result,
                              std::error_code& ec) noexcept {
  // parse token SP
  const char* const save_first = beg;
  while (true) {
    if (beg >= last) {
      ec = Error::kNeedMore;
      return;
    }
    if (!IsTokenChar(*beg)) {
      break;
    }
    ++beg;
  }
  if (!std::isspace(*beg) || beg == save_first) {
    ec = Error::kBadMethod;
    return;
  }
  result = MakeString(save_first, beg);
  ++beg;
}

void BasicParser::ParseUri(const char*& first, const char* last,
                           std::string_view& result,
                           std::error_code& ec) noexcept {
  // parse target SP
  const char* const save_first = first;
  while (true) {
    if (first >= last) {
      ec = Error::kNeedMore;
      return;
    }
    if (!IsPathChar(*first)) {
      break;
    }
    ++first;
  }
  if (std::isspace(*first) == 0 || first == save_first) {
    ec = Error::kBadTarget;
    return;
  }
  result = MakeString(save_first, first);
  ++first;
}

void BasicParser::ParseVersion(const char*& it, const char* last, int& result,
                               std::error_code& ec) noexcept {
  if (it + 8 > last) {
    ec = Error::kNeedMore;
    return;
  }
  if (*it++ != 'H') {
    ec = Error::kBadVersion;
    return;
  }
  if (*it++ != 'T') {
    ec = Error::kBadVersion;
    return;
  }
  if (*it++ != 'T') {
    ec = Error::kBadVersion;
    return;
  }
  if (*it++ != 'P') {
    ec = Error::kBadVersion;
    return;
  }
  if (*it++ != '/') {
    ec = Error::kBadVersion;
    return;
  }
  if (!std::isdigit(*it)) {
    ec = Error::kBadVersion;
    return;
  }
  result = 10 * (*it++ - '0');
  if (*it++ != '.') {
    ec = Error::kBadVersion;
    return;
  }
  if (!std::isdigit(*it)) {
    ec = Error::kBadVersion;
    return;
  }
  result += *it++ - '0';
}

void BasicParser::ParseStatus(const char*& it, const char* last,
                              uint16_t& result, std::error_code& ec) noexcept {
  // parse 3(digit) SP
  if (it + 4 > last) {
    ec = Error::kNeedMore;
    return;
  }
  if (!std::isdigit(*it)) {
    ec = Error::kBadStatus;
    return;
  }
  result = 100 * (*it++ - '0');
  if (!std::isdigit(*it)) {
    ec = Error::kBadStatus;
    return;
  }
  result += 10 * (*it++ - '0');
  if (!std::isdigit(*it)) {
    ec = Error::kBadStatus;
    return;
  }
  result += *it++ - '0';
  if (*it++ != ' ') {
    ec = Error::kBadStatus;
    return;
  }
}

void BasicParser::ParseReason(const char*& it, const char* last,
                              std::string_view& result,
                              std::error_code& ec) noexcept {
  auto const first = it;
  const char* token_end = nullptr;
  auto p = ParseTokenToEol(it, last, token_end, ec);
  if (ec) {
    return;
  }
  // empty reason not allowed
  if (!p) {
    ec = Error::kBadReason;
    return;
  }
  result = MakeString(first, token_end);
  it = p;
}

// TODO(xm): 改名
void BasicParser::ParseField(const char*& p, const char* last,
                             std::string_view& name, std::string_view& value,
                             beast::detail::char_buffer<max_obs_fold>& buf,
                             std::error_code& ec) noexcept {
  /*  header-field    = field-name ":" OWS field-value OWS

      field-name      = token
      field-value     = *( field-content / obs-fold )
      field-content   = field-vchar [ 1*( SP / HTAB ) field-vchar ]
      field-vchar     = VCHAR / obs-text

      obs-fold        = CRLF 1*( SP / HTAB )
               obsolete line folding
               see Section 3.2.4

      token           = 1*<any CHAR except CTLs or separators>
      CHAR            = <any US-ASCII character (octets 0 - 127)>
      sep             = "(" | ")" | "<" | ">" | "@"
                      | "," | ";" | ":" | "\" | <">
                      | "/" | "[" | "]" | "?" | "="
                      | "{" | "}" | SP | HT
  */
  static const char* is_token =
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
      "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
      "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  // name
  BOOST_ALIGNMENT(16)
  static const char ranges1[] =
      "\x00 "  /* control chars and up to SP */
      "\"\""   /* 0x22 */
      "()"     /* 0x28,0x29 */
      ",,"     /* 0x2c */
      "//"     /* 0x2f */
      ":@"     /* 0x3a-0x40 */
      "[]"     /* 0x5b-0x5d */
      "{\377"; /* 0x7b-0xff */
  auto first = p;
  bool found{false};
  // std::tie(p, found) = find_fast(p, last, ranges1, sizeof(ranges1) - 1);
  if (!found && p >= last) {
    ec = Error::kNeedMore;
    return;
  }

  // find field name
  while (true) {
    if (*p == ':') {
      break;
    }
    if (!is_token[static_cast<unsigned char>(*p)]) {
      ec = Error::kBadField;
      return;
    }
    ++p;
    if (p >= last) {
      ec = Error::kNeedMore;
      return;
    }
  }
  if (p == first) {
    // empty name
    ec = Error::kBadField;
    return;
  }
  name = MakeString(first, p);

  // eat ':'
  ++p;

  const char* token_end = nullptr;
  while (true) {
    // eat leading ' ' and '\t'
    while (true) {
      if (p >= last) {
        ec = Error::kNeedMore;
        return;
      }
      if (!(*p == ' ' || *p == '\t')) {
        break;
      }
      ++p;
    }

    // parse to CRLF
    first = p;
    p = ParseTokenToEol(p, last, token_end, ec);
    if (ec) {
      return;
    }

    // empty field value
    if (!p) {
      ec = Error::kBadValue;
      return;
    }
    // Look 1 char past the CRLF to handle obs-fold.
    if (p >= last) {
      ec = Error::kNeedMore;
      return;
    }
    // TODO(xm): BUG
    token_end = TrimTail(first, token_end);
    if (*p != ' ' && *p != '\t') {
      value = MakeString(first, token_end);
      return;
    }
    ++p;
    if (token_end != first) {
      break;
    }
  }

  buf.clear();
  if (!buf.try_append(first, token_end)) {
    ec = Error::kHeaderLimit;
    return;
  }

  BOOST_ASSERT(!buf.empty());
  for (;;) {
    // eat leading ' ' and '\t'
    for (;; ++p) {
      if (p + 1 > last) {
        ec = Error::kNeedMore;

        return;
      }
      if (!(*p == ' ' || *p == '\t'))
        break;
    }
    // parse to CRLF
    first = p;
    p = ParseTokenToEol(p, last, token_end, ec);
    if (ec)
      return;
    if (!p) {
      ec = Error::kBadValue;
      return;
    }
    // Look 1 char past the CRLF to handle obs-fold.
    if (p + 1 > last) {
      ec = Error::kNeedMore;
      return;
    }
    token_end = TrimTail(token_end, first);
    if (first != token_end) {
      if (!buf.try_push_back(' ') || !buf.try_append(first, token_end)) {
        ec = Error::kHeaderLimit;
        return;
      }
    }
    if (*p != ' ' && *p != '\t') {
      value = {buf.data(), buf.size()};
      return;
    }
    ++p;
  }
}

void BasicParser::ParseChunkExtensions(const char*& it, const char* last,
                                       std::error_code& ec) noexcept {
/*
    chunk-ext       = *( BWS  ";" BWS chunk-ext-name [ BWS  "=" BWS
   chunk-ext-val ] ) BWS             = *( SP / HTAB ) ; "Bad White Space"
    chunk-ext-name  = token
    chunk-ext-val   = token / quoted-string
    token           = 1*tchar
    quoted-string   = DQUOTE *( qdtext / quoted-pair ) DQUOTE
    qdtext          = HTAB / SP / "!" / %x23-5B ; '#'-'[' / %x5D-7E ; ']'-'~' /
   obs-text quoted-pair     = "\" ( HTAB / SP / VCHAR / obs-text ) obs-text =
   %x80-FF

    https://www.rfc-editor.org/errata_search.php?rfc=7230&eid=4667
*/
loop:
  if (it == last) {
    ec = Error::kNeedMore;
    return;
  }
  if (*it != ' ' && *it != '\t' && *it != ';') {
    return;
  }
  // BWS
  if (*it == ' ' || *it == '\t') {
    for (;;) {
      ++it;
      if (it == last) {
        ec = Error::kNeedMore;

        return;
      }
      if (*it != ' ' && *it != '\t') {
        break;
      }
    }
  }
  // ';'
  if (*it != ';') {
    ec = Error::kBadChunkExtension;
    return;
  }
semi:
  ++it;  // skip ';'
  // BWS
  for (;;) {
    if (it == last) {
      ec = Error::kNeedMore;
      return;
    }
    if (*it != ' ' && *it != '\t') {
      break;
    }
    ++it;
  }
  // chunk-ext-name
  if (!IsTokenChar(*it)) {
    ec = Error::kBadChunkExtension;
    return;
  }
  for (;;) {
    ++it;
    if (it == last) {
      ec = Error::kNeedMore;
      return;
    }
    if (!IsTokenChar(*it)) {
      break;
    }
  }
  // BWS [ ";" / "=" ]
  {
    bool bws{};
    if (*it == ' ' || *it == '\t') {
      for (;;) {
        ++it;
        if (it == last) {
          ec = Error::kNeedMore;
          return;
        }
        if (*it != ' ' && *it != '\t') {
          break;
        }
      }
      bws = true;
    } else {
      bws = false;
    }
    if (*it == ';') {
      goto semi;
    }

    if (*it != '=') {
      if (bws) {
        ec = Error::kBadChunkExtension;
      }

      return;
    }
    ++it;  // skip '='
  }
  // BWS
  for (;;) {
    if (it == last) {
      ec = Error::kNeedMore;
      return;
    }
    if (*it != ' ' && *it != '\t') {
      break;
    }
    ++it;
  }
  // chunk-ext-val
  if (*it != '"') {
    // token
    if (!IsTokenChar(*it)) {
      ec = Error::kBadChunkExtension;
      return;
    }
    for (;;) {
      ++it;
      if (it == last) {
        ec = Error::kNeedMore;
        return;
      }
      if (!IsTokenChar(*it)) {
        break;
      }
    }
  } else {
    // quoted-string
    for (;;) {
      ++it;
      if (it == last) {
        ec = Error::kNeedMore;
        return;
      }
      if (*it == '"') {
        break;
      }
      if (*it == '\\') {
        ++it;
        if (it == last) {
          ec = Error::kNeedMore;
          return;
        }
      }
    }
    ++it;
  }
  goto loop;
}

}  // namespace net::http
