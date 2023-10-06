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
#include <system_error> // NOLINT

namespace net::http1 {
  // Error codes returned from HTTP algorithms and operations.
  enum class Error {
    kSuccess = 0,
    kEndOfStream,
    kPartialMessage,
    kNeedMore,
    kUnexpectedBody,
    kNeedBuffer,
    kEndOfChunk,
    kBufferOverflow,
    kHeaderLimit,
    kBodyLimit,
    kBadAlloc,
    kBadLineEnding,
    kEmptyMethod,
    kBadMethod,
    kBadUri,
    kBadScheme,
    kBadHost,
    kBadPort,
    kBadPath,
    kBadParams,
    kBadVersion,
    kBadStatus,
    kBadReason,
    kBadHeader,
    kBadHeaderName,
    kEmptyHeaderName,
    kEmptyHeaderValue,
    kBadHeaderValue,
    kBadContentLength,
    kBadTransferEncoding,
    kBadChunk,
    kBadChunkExtension,
    kBadObsFold,
    kMultipleContentLength,
    kStaleParser,
    kShortRead,
    kRecvTimeoutWithReceivingNothing,
    kRecvRequestLineTimeout,
    kRecvRequestHeadersTimeout,
    kRecvRequestBodyTimeout,
  };

  class HttpErrorCategory : public std::error_category {
   public:
    [[nodiscard]] const char* name() const noexcept override {
      return "net.http";
    }

    HttpErrorCategory() = default;

    [[nodiscard]] std::string message(int err) const override {
      switch (static_cast<Error>(err)) {
      case Error::kEndOfStream:
        return "end of stream";
      case Error::kPartialMessage:
        return "partial message";
      case Error::kNeedMore:
        return "need more";
      case Error::kUnexpectedBody:
        return "unexpected body";
      case Error::kNeedBuffer:
        return "need buffer";
      case Error::kEndOfChunk:
        return "end of chunk";
      case Error::kBufferOverflow:
        return "buffer overflow";
      case Error::kHeaderLimit:
        return "header limit exceeded";
      case Error::kBodyLimit:
        return "body limit exceeded";
      case Error::kBadAlloc:
        return "bad alloc";
      case Error::kBadLineEnding:
        return "bad line ending";
      case Error::kEmptyMethod:
        return "empty method";
      case Error::kBadMethod:
        return "bad method";
      case Error::kBadUri:
        return "bad uri";
      case Error::kBadScheme:
        return "bad scheme";
      case Error::kBadHost:
        return "bad host";
      case Error::kBadPort:
        return "bad port";
      case Error::kBadPath:
        return "bad path";
      case Error::kBadParams:
        return "bad params";
      case Error::kBadVersion:
        return "bad version";
      case Error::kBadStatus:
        return "bad status";
      case Error::kBadReason:
        return "bad reason";
      case Error::kBadHeader:
        return "bad header";
      case Error::kEmptyHeaderName:
        return "empty header name";
      case Error::kEmptyHeaderValue:
        return "empty header value";
      case Error::kBadHeaderName:
        return "bad header name";
      case Error::kBadHeaderValue:
        return "bad header value";
      case Error::kBadContentLength:
        return "bad Content-Length";
      case Error::kBadTransferEncoding:
        return "bad Transfer-Encoding";
      case Error::kBadChunk:
        return "bad chunk";
      case Error::kBadChunkExtension:
        return "bad chunk extension";
      case Error::kBadObsFold:
        return "bad obs-fold";
      case Error::kMultipleContentLength:
        return "multiple Content-Length";
      case Error::kStaleParser:
        return "stale parser";
      case Error::kShortRead:
        return "unexpected eof in body";
      case Error::kRecvTimeoutWithReceivingNothing:
        return "receive timeout with receiving nothing";
      case Error::kRecvRequestLineTimeout:
        return "receive request line timeout";
      case Error::kRecvRequestHeadersTimeout:
        return "receive request headers timeout";
      case Error::kRecvRequestBodyTimeout:
        return "receive request body timeout";

      default:
        return "beast.http Error";
      }
    }

    [[nodiscard]] std::error_condition default_error_condition(int err) const noexcept override {
      return std::error_condition{err, *this};
    }

    [[nodiscard]] bool
      equivalent(int err, std::error_condition const & condition) const noexcept override {
      return condition.value() == err && &condition.category() == this;
    }

    [[nodiscard]] bool equivalent(const std::error_code& error, int err) const noexcept override {
      return error.value() == err && &error.category() == this;
    }
  };

  inline std::error_code make_error_code(net::http1::Error err) { // NOLINT
    static net::http1::HttpErrorCategory category{};
    return {static_cast<std::underlying_type<net::http1::Error>::type>(err), category};
  }

} // namespace net::http1

namespace std {
  template <>
  struct is_error_code_enum<net::http1::Error> {
    static bool const value = true; // NOLINT
  };
} // namespace std
