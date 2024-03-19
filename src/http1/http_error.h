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
  enum class error {
    success = 0,
    end_of_stream,
    kPartialMessage,
    need_more,
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
    kInvalidResponse,
    kRecvTimeout,
    recv_request_timeout_with_nothing,
    recv_request_line_timeout,
    recv_request_headers_timeout,
    recv_request_body_timeout,
    kSendTimeout,
    kSendResponseTimeoutWithNothing,
    kSendResponseLineAndHeadersTimeout,
    kSendResponseBodyTimeout
  };

  class HttpErrorCategory : public std::error_category {
   public:
    [[nodiscard]] const char* name() const noexcept override {
      return "net.http";
    }

    HttpErrorCategory() = default;

    [[nodiscard]] std::string message(int err) const override {
      switch (static_cast<error>(err)) {
      case error::end_of_stream:
        return "end of stream";
      case error::kPartialMessage:
        return "partial message";
      case error::need_more:
        return "need more";
      case error::kUnexpectedBody:
        return "unexpected body";
      case error::kNeedBuffer:
        return "need buffer";
      case error::kEndOfChunk:
        return "end of chunk";
      case error::kBufferOverflow:
        return "buffer overflow";
      case error::kHeaderLimit:
        return "header limit exceeded";
      case error::kBodyLimit:
        return "body limit exceeded";
      case error::kBadAlloc:
        return "bad alloc";
      case error::kBadLineEnding:
        return "bad line ending";
      case error::kEmptyMethod:
        return "empty method";
      case error::kBadMethod:
        return "bad method";
      case error::kBadUri:
        return "bad uri";
      case error::kBadScheme:
        return "bad scheme";
      case error::kBadHost:
        return "bad host";
      case error::kBadPort:
        return "bad port";
      case error::kBadPath:
        return "bad path";
      case error::kBadParams:
        return "bad params";
      case error::kBadVersion:
        return "bad version";
      case error::kBadStatus:
        return "bad status";
      case error::kBadReason:
        return "bad reason";
      case error::kBadHeader:
        return "bad header";
      case error::kEmptyHeaderName:
        return "empty header name";
      case error::kEmptyHeaderValue:
        return "empty header value";
      case error::kBadHeaderName:
        return "bad header name";
      case error::kBadHeaderValue:
        return "bad header value";
      case error::kBadContentLength:
        return "bad Content-Length";
      case error::kBadTransferEncoding:
        return "bad Transfer-Encoding";
      case error::kBadChunk:
        return "bad chunk";
      case error::kBadChunkExtension:
        return "bad chunk extension";
      case error::kBadObsFold:
        return "bad obs-fold";
      case error::kMultipleContentLength:
        return "multiple Content-Length";
      case error::kStaleParser:
        return "stale parser";
      case error::kShortRead:
        return "unexpected eof in body";
      case error::kInvalidResponse:
        return "invalid response";
      case error::kRecvTimeout:
        return "receive timeout";
      case error::recv_request_timeout_with_nothing:
        return "receive request timeout with nothing";
      case error::recv_request_line_timeout:
        return "receive request line timeout";
      case error::recv_request_headers_timeout:
        return "receive request headers timeout";
      case error::recv_request_body_timeout:
        return "receive request body timeout";
      case error::kSendTimeout:
        return "send timeout";
      case error::kSendResponseTimeoutWithNothing:
        return "send response timeout with nothing";
      case error::kSendResponseLineAndHeadersTimeout:
        return "send response headers timeout";
      case error::kSendResponseBodyTimeout:
        return "send response body timeout";

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

  inline std::error_code make_error_code(net::http1::error err) { // NOLINT
    static net::http1::HttpErrorCategory category{};
    return {static_cast<std::underlying_type_t<net::http1::error>>(err), category};
  }

} // namespace net::http1

namespace std {
  template <>
  struct is_error_code_enum<net::http1::error> {
    static bool const value = true; // NOLINT
  };
} // namespace std
