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
#pragma once

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1000

#include <array>
#include <charconv>
#include <string>
#include <string_view>

#include <magic_enum.hpp>

namespace net::http1 {

  enum class http_scheme {
    http,
    https,
    unknown
  };

  static constexpr std::array<std::string_view, 5> kHttpVersions =
    {"HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/3.0", "UNKNOWN_HTTP_VERSION"};

  enum class http_version {
    http10,
    http11,
    http20,
    http30,
    unknown
  };

  constexpr http_version to_http_version(int total) {
    if (total == 10) {
      return http_version::http10;
    }
    if (total == 11) {
      return http_version::http11;
    }
    if (total == 20) {
      return http_version::http20;
    }
    if (total == 30) {
      return http_version::http30;
    }
    return http_version::unknown;
  }

  constexpr http_version to_http_version(int major, int minor) {
    return to_http_version(major * 10 + minor);
  }

  constexpr std::string_view HttpVersionToString(http_version version) noexcept {
    if (version == http_version::http10) {
      return kHttpVersions[0];
    }
    if (version == http_version::http11) {
      return kHttpVersions[1];
    }
    if (version == http_version::http20) {
      return kHttpVersions[2];
    }
    if (version == http_version::http30) {
      return kHttpVersions[3];
    }
    return kHttpVersions[4];
  }

  enum class http_method {
    get,
    head,
    post,
    put,
    delete_, // conflict with delete keyword
    trace,
    control,
    purge,
    options,
    connect,
    unknown
  };

  constexpr std::string_view HttpMethodToString(http_method method) noexcept {
    switch (method) {
    case http_method::get:
      return "GET";
    case http_method::head:
      return "HEAD";
    case http_method::post:
      return "POST";
    case http_method::put:
      return "PUT";
    case http_method::delete_:
      return "DELETE";
    case http_method::trace:
      return "TRACE";
    case http_method::control:
      return "CONTROL";
    case http_method::purge:
      return "PURGE";
    case http_method::options:
      return "OPTIONS";
    case http_method::connect:
      return "CONNECT";
    default:
      return "UNKNOWN";
    }
  }

  constexpr http_method ToHttpMethod(std::string_view method) noexcept {
    if (method == "GET") {
      return http_method::get;
    }
    if (method == "HEAD") {
      return http_method::head;
    }
    if (method == "POST") {
      return http_method::post;
    }
    if (method == "PUT") {
      return http_method::put;
    }
    if (method == "DELETE") {
      return http_method::delete_;
    }
    if (method == "TRACE") {
      return http_method::trace;
    }
    if (method == "CONTROL") {
      return http_method::control;
    }
    if (method == "PURGE") {
      return http_method::purge;
    }
    if (method == "OPTIONS") {
      return http_method::options;
    }

    return http_method::unknown;
  }

  enum class http_status_code {
    unknown = 0,
    kContinue = 100,
    kOK = 200,
    kCreate = 201,
    kAccepted = 202,
    kNonAuthoritative = 203,
    kNoContent = 204,
    kResetContent = 205,
    kPartialContent = 206,
    kMultiStatus = 207,
    kMultipleChoices = 300,
    kMovedPermanently = 301,
    kMovedTemporarily = 302,
    kSeeOther = 303,
    kNotModified = 304,
    kUseProxy = 305,
    kTemporaryRedirect = 307,
    kPermanentRedirect = 308,
    kBadRequest = 400,
    kUnauthorized = 401,
    kPaymentRequired = 402,
    kForbidden = 403,
    kNotFound = 404,
    kMethodNotAllowed = 405,
    kNotAcceptable = 406,
    kRequestTimeout = 408,
    kLengthRequired = 411,
    kPreconditionFailed = 412,
    kRequestEntityTooLarge = 413,
    kRequestUriTooLarge = 414,
    kUnsupportedMediaType = 415,
    kRangeNotSatisfiable = 416,
    kExpectationFailed = 417,
    kUnprocessableEntity = 422,
    kLocked = 423,
    kFailedDependency = 424,
    kUpgradeRequired = 426,
    kUnavailableForLegalReasons = 451,
    kInternalServerError = 500,
    kNotImplemented = 501,
    kBadGateway = 502,
    kServiceUnavailable = 503,
    kGatewayTimeout = 504,
    kVersionNotSupported = 505,
    kVariantAlsoVaries = 506,
    kInsufficientStorage = 507,
    kNotExtended = 510,
    kFrequencyCapping = 514,
    kScriptServerError = 544,
  };

  inline std::string HttpStatusCodeToString(http_status_code code) noexcept {
    // TODO(xiaoming): refactor a sufficient way
    return std::to_string(static_cast<uint32_t>(code));
  }

  constexpr std::string_view HttpStatusReason(http_status_code code) noexcept {
    switch (code) {
    case http_status_code::unknown:
      return "Unknown Status";
    case http_status_code::kContinue:
      return "Continue";
    case http_status_code::kOK:
      return "OK";
    case http_status_code::kCreate:
      return "Created";
    case http_status_code::kAccepted:
      return "Accepted";
    case http_status_code::kNonAuthoritative:
      return "Non-Authoritative Information";
    case http_status_code::kNoContent:
      return "No Content";
    case http_status_code::kResetContent:
      return "Reset Content";
    case http_status_code::kPartialContent:
      return "Partial Content";
    case http_status_code::kMultiStatus:
      return "Multi-Status";
    case http_status_code::kMultipleChoices:
      return "Multiple Choices";
    case http_status_code::kMovedPermanently:
      return "Moved Permanently";
    case http_status_code::kMovedTemporarily:
      return "Found";
    case http_status_code::kSeeOther:
      return "See Other";
    case http_status_code::kNotModified:
      return "Not Modified";
    case http_status_code::kUseProxy:
      return "Use Proxy";
    case http_status_code::kTemporaryRedirect:
      return "Temporary Redirect";
    case http_status_code::kPermanentRedirect:
      return "Permanent Redirect";
    case http_status_code::kBadRequest:
      return "Bad Request";
    case http_status_code::kUnauthorized:
      return "Authorization Required";
    case http_status_code::kPaymentRequired:
      return "Payment Required";
    case http_status_code::kForbidden:
      return "Forbidden";
    case http_status_code::kNotFound:
      return "Not Found";
    case http_status_code::kMethodNotAllowed:
      return "Method Not Allowed";
    case http_status_code::kNotAcceptable:
      return "Not Acceptable";
    case http_status_code::kRequestTimeout:
      return "Request Time-out";
    case http_status_code::kLengthRequired:
      return "Length Required";
    case http_status_code::kPreconditionFailed:
      return "Precondition Failed";
    case http_status_code::kRequestEntityTooLarge:
      return "Request Entity Too Large";
    case http_status_code::kRequestUriTooLarge:
      return "Request-URI Too Large";
    case http_status_code::kUnsupportedMediaType:
      return "Unsupported Media Type";
    case http_status_code::kRangeNotSatisfiable:
      return "Request Range Not Satisfiable";
    case http_status_code::kExpectationFailed:
      return "Expectation Failed";
    case http_status_code::kUnprocessableEntity:
      return "Unprocessable Entity";
    case http_status_code::kLocked:
      return "Locked";
    case http_status_code::kFailedDependency:
      return "Failed Dependency";
    case http_status_code::kUpgradeRequired:
      return "Upgrade Required";
    case http_status_code::kUnavailableForLegalReasons:
      return "Unavailable For Legal Reasons";
    case http_status_code::kInternalServerError:
      return "Internal Error";
    case http_status_code::kNotImplemented:
      return "Method Not Implemented";
    case http_status_code::kBadGateway:
      return "Bad Gateway";
    case http_status_code::kServiceUnavailable:
      return "Service Temporarily Unavailable";
    case http_status_code::kGatewayTimeout:
      return "Gateway Time-out";
    case http_status_code::kVersionNotSupported:
      return "HTTP Version Not Supported";
    case http_status_code::kVariantAlsoVaries:
      return "Variant Also Negotiates";
    case http_status_code::kInsufficientStorage:
      return "Insufficent Storage";
    case http_status_code::kNotExtended:
      return "Not Extended";
    case http_status_code::kFrequencyCapping:
      return "Frequency Capped";
    default:
      return "Unknown Status Code";
    }
  }

  inline http_status_code to_http_status_code(std::string_view status) noexcept {
    int value{0};
    auto [ptr, ec]{std::from_chars(status.data(), status.data() + status.size(), value)};
    if (ec != std::errc()) {
      return http_status_code::unknown;
    }

    return magic_enum::enum_cast<http_status_code>(value).value_or(http_status_code::unknown);
  }

  static constexpr std::string_view kHttpHeaderHost = "Host";
  static constexpr std::string_view kHttpHeaderContentLength = "Content-Length";
  static constexpr std::string_view kHttpHeaderIfModifiedSince = "If-Modified-Since";
  static constexpr std::string_view kHttpHeaderEtag = "Etag";
  static constexpr std::string_view kHttpHeaderAcceptEncoding = "Accept-Encoding";
  static constexpr std::string_view kHttpHeaderLastModified = "Last-Modified";
  static constexpr std::string_view kHttpHeaderContentRange = "Content-Range";
  static constexpr std::string_view kHttpHeaderContentType = "Content-Type";
  static constexpr std::string_view kHttpHeaderTransferEncoding = "Transfer-Encoding";
  static constexpr std::string_view kHttpHeaderContentEncoding = "Content-Encoding";
  static constexpr std::string_view kHttpHeaderConnection = "Connection";
  static constexpr std::string_view kHttpHeaderRange = "Range";
  static constexpr std::string_view kHttpHeaderServer = "Server";
  static constexpr std::string_view kHttpHeaderDate = "Date";
  static constexpr std::string_view kHttpHeaderLocation = "Location";
  static constexpr std::string_view kHttpHeaderExpect = "Expect";
  static constexpr std::string_view kHttpHeaderCacheControl = "Cache-Control";
  static constexpr std::string_view kHttpHeaderCacheTag = "Cache-Tag";
  static constexpr std::string_view kHttpHeaderExpires = "Expires";
  static constexpr std::string_view kHttpHeaderReferer = "Referer";
  static constexpr std::string_view kHttpHeaderUserAgent = "User-Agent";
  static constexpr std::string_view kHttpHeaderCookie = "Cookie";
  static constexpr std::string_view kHttpHeaderXForwardedFor = "X-Forwarded-For";
  static constexpr std::string_view kHttpHeaderAcceptLanguage = "Accept-Language";
  static constexpr std::string_view kHttpHeaderAcceptCharset = "Accept-Charset";
  static constexpr std::string_view kHttpHeaderAcceptRanges = "Accept-Ranges";
  static constexpr std::string_view kHttpHeaderSetCookie = "Set-Cookie";
  static constexpr std::string_view kHttpHeaderVia = "Via";
  static constexpr std::string_view kHttpHeaderPragma = "Pragma";
  static constexpr std::string_view kHttpHeaderUpgrade = "Upgrade";
  static constexpr std::string_view kHttpHeaderIfNoneMatch = "If-None-Match";
  static constexpr std::string_view kHttpHeaderIfMatch = "If-Match";
  static constexpr std::string_view kHttpHeaderIfRange = "If-Range";
  static constexpr std::string_view kHttpHeaderAccept = "Accept";
  static constexpr std::string_view kHttpHeaderAge = "Age";
  static constexpr std::string_view kHttpHeaderChunked = "chunked";
  static constexpr std::string_view kHttpHeaderIdentity = "identity";
  static constexpr std::string_view kHttpHeaderKeepalive = "keep-alive";
  static constexpr std::string_view kHttpHeaderClose = "close";

}; // namespace net::http1
