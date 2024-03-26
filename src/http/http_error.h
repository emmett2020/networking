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

namespace net::http {
  // Error codes returned from HTTP algorithms and operations.
  enum class error {
    success = 0,
    end_of_stream,
    partial_message,
    need_more,
    unexpected_body,
    need_buffer,
    end_of_chunk,
    buffer_overflow,
    header_limit,
    body_limit,
    bad_alloc,
    bad_line_ending,
    empty_method,
    unknown_method,
    bad_method,
    bad_uri,
    bad_scheme,
    empty_host,
    bad_host,
    too_big_port,
    bad_port,
    bad_path,
    bad_params,
    bad_version,
    unknown_status,
    bad_status,
    bad_reason,
    bad_header,
    bad_header_name,
    empty_header_name,
    empty_header_value,
    bad_header_value,
    bad_content_length,
    bad_transfer_encoding,
    bad_chunk,
    bad_chunk_extension,
    bad_obs_fold,
    multiple_content_length,
    stale_parser,
    short_read,
    invalid_response,
    recv_timeout,
    recv_request_timeout_with_nothing,
    recv_request_line_timeout,
    recv_request_headers_timeout,
    recv_request_body_timeout,
    send_timeout,
    send_response_timeout_with_nothing,
    send_response_line_and_headers_timeout,
    send_response_body_timeout
  };

  class http_error_category : public std::error_category {
   public:
    [[nodiscard]] const char* name() const noexcept override {
      return "net.http";
    }

    http_error_category() = default;

    [[nodiscard]] std::string message(int err) const override {
      switch (static_cast<error>(err)) {
      case error::end_of_stream:
        return "end of stream";
      case error::partial_message:
        return "partial message";
      case error::need_more:
        return "need more";
      case error::unexpected_body:
        return "unexpected body";
      case error::need_buffer:
        return "need buffer";
      case error::end_of_chunk:
        return "end of chunk";
      case error::buffer_overflow:
        return "buffer overflow";
      case error::header_limit:
        return "header limit exceeded";
      case error::body_limit:
        return "body limit exceeded";
      case error::bad_alloc:
        return "bad alloc";
      case error::bad_line_ending:
        return "bad line ending";
      case error::empty_method:
        return "empty method";
      case error::bad_method:
        return "bad method";
      case error::bad_uri:
        return "bad uri";
      case error::bad_scheme:
        return "bad scheme";
      case error::bad_host:
        return "bad host";
      case error::bad_port:
        return "bad port";
      case error::bad_path:
        return "bad path";
      case error::bad_params:
        return "bad params";
      case error::bad_version:
        return "bad version";
      case error::bad_status:
        return "bad status";
      case error::bad_reason:
        return "bad reason";
      case error::bad_header:
        return "bad header";
      case error::empty_header_name:
        return "empty header name";
      case error::empty_header_value:
        return "empty header value";
      case error::bad_header_name:
        return "bad header name";
      case error::bad_header_value:
        return "bad header value";
      case error::bad_content_length:
        return "bad Content-Length";
      case error::bad_transfer_encoding:
        return "bad Transfer-Encoding";
      case error::bad_chunk:
        return "bad chunk";
      case error::bad_chunk_extension:
        return "bad chunk extension";
      case error::bad_obs_fold:
        return "bad obs-fold";
      case error::multiple_content_length:
        return "multiple Content-Length";
      case error::stale_parser:
        return "stale parser";
      case error::short_read:
        return "unexpected eof in body";
      case error::invalid_response:
        return "invalid response";
      case error::recv_timeout:
        return "receive timeout";
      case error::recv_request_timeout_with_nothing:
        return "receive request timeout with nothing";
      case error::recv_request_line_timeout:
        return "receive request line timeout";
      case error::recv_request_headers_timeout:
        return "receive request headers timeout";
      case error::recv_request_body_timeout:
        return "receive request body timeout";
      case error::send_timeout:
        return "send timeout";
      case error::send_response_timeout_with_nothing:
        return "send response timeout with nothing";
      case error::send_response_line_and_headers_timeout:
        return "send response headers timeout";
      case error::send_response_body_timeout:
        return "send response body timeout";
      default:
        return "net.http Error";
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

  inline std::error_code make_error_code(net::http::error err) { // NOLINT
    static net::http::http_error_category category{};
    return {static_cast<std::underlying_type_t<net::http::error>>(err), category};
  }

} // namespace net::http

namespace std {
  template <>
  struct is_error_code_enum<net::http::error> {
    static bool const value = true; // NOLINT
  };
} // namespace std
