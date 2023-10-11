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
#include <cstring>
#include <string>
#include <string_view>

#include <magic_enum.hpp>

namespace net::http1 {

  enum class HttpScheme {
    kHttp,
    kHttps,
    kUnknown
  };

  static constexpr std::array<std::string_view, 5> kHttpVersions =
    {"HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/3.0", "UNKNOWN_HTTP_VERSION"};

  enum class HttpVersion {
    kHttp10,
    kHttp11,
    kHttp20,
    kHttp30,
    kUnknown
  };

  constexpr HttpVersion ToHttpVersion(int total) {
    if (total == 10) {
      return HttpVersion::kHttp10;
    }
    if (total == 11) {
      return HttpVersion::kHttp11;
    }
    if (total == 20) {
      return HttpVersion::kHttp20;
    }
    if (total == 30) {
      return HttpVersion::kHttp30;
    }
    return HttpVersion::kUnknown;
  }

  constexpr HttpVersion ToHttpVersion(int major, int minor) {
    return ToHttpVersion(major * 10 + minor);
  }

  constexpr std::string_view HttpVersionToString(HttpVersion version) noexcept {
    if (version == HttpVersion::kHttp10) {
      return kHttpVersions[0];
    }
    if (version == HttpVersion::kHttp11) {
      return kHttpVersions[1];
    }
    if (version == HttpVersion::kHttp20) {
      return kHttpVersions[2];
    }
    if (version == HttpVersion::kHttp30) {
      return kHttpVersions[3];
    }
    return kHttpVersions[4];
  }

  enum class HttpMethod {
    kGet,
    kHead,
    kPost,
    kPut,
    kDelete,
    kTrace,
    kControl,
    kPurge,
    kOptions,
    kConnect,
    kUnknown
  };

  constexpr std::string_view HttpMethodToString(HttpMethod method) noexcept {
    switch (method) {
    case HttpMethod::kGet:
      return "GET";
    case HttpMethod::kHead:
      return "HEAD";
    case HttpMethod::kPost:
      return "POST";
    case HttpMethod::kPut:
      return "PUT";
    case HttpMethod::kDelete:
      return "DELETE";
    case HttpMethod::kTrace:
      return "TRACE";
    case HttpMethod::kControl:
      return "CONTROL";
    case HttpMethod::kPurge:
      return "PURGE";
    case HttpMethod::kOptions:
      return "OPTIONS";
    case HttpMethod::kConnect:
      return "CONNECT";
    default:
      return "UNKNOWN";
    }
  }

  constexpr HttpMethod ToHttpMethod(std::string_view method) noexcept {
    if (method == "GET") {
      return HttpMethod::kGet;
    }
    if (method == "HEAD") {
      return HttpMethod::kHead;
    }
    if (method == "POST") {
      return HttpMethod::kPost;
    }
    if (method == "PUT") {
      return HttpMethod::kPut;
    }
    if (method == "DELETE") {
      return HttpMethod::kDelete;
    }
    if (method == "TRACE") {
      return HttpMethod::kTrace;
    }
    if (method == "CONTROL") {
      return HttpMethod::kControl;
    }
    if (method == "PURGE") {
      return HttpMethod::kPurge;
    }
    if (method == "OPTIONS") {
      return HttpMethod::kOptions;
    }

    return HttpMethod::kUnknown;
  }

  enum class HttpStatusCode {
    kUnknown = 0,
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

  inline std::string HttpStatusCodeToString(HttpStatusCode code) noexcept {
    // TODO(xiaoming): refactor a sufficient way
    return std::to_string(static_cast<uint32_t>(code));
  }

  constexpr std::string_view HttpStatusReason(HttpStatusCode code) noexcept {
    switch (code) {
    case HttpStatusCode::kUnknown:
      return "Unknown Status";
    case HttpStatusCode::kContinue:
      return "Continue";
    case HttpStatusCode::kOK:
      return "OK";
    case HttpStatusCode::kCreate:
      return "Created";
    case HttpStatusCode::kAccepted:
      return "Accepted";
    case HttpStatusCode::kNonAuthoritative:
      return "Non-Authoritative Information";
    case HttpStatusCode::kNoContent:
      return "No Content";
    case HttpStatusCode::kResetContent:
      return "Reset Content";
    case HttpStatusCode::kPartialContent:
      return "Partial Content";
    case HttpStatusCode::kMultiStatus:
      return "Multi-Status";
    case HttpStatusCode::kMultipleChoices:
      return "Multiple Choices";
    case HttpStatusCode::kMovedPermanently:
      return "Moved Permanently";
    case HttpStatusCode::kMovedTemporarily:
      return "Found";
    case HttpStatusCode::kSeeOther:
      return "See Other";
    case HttpStatusCode::kNotModified:
      return "Not Modified";
    case HttpStatusCode::kUseProxy:
      return "Use Proxy";
    case HttpStatusCode::kTemporaryRedirect:
      return "Temporary Redirect";
    case HttpStatusCode::kPermanentRedirect:
      return "Permanent Redirect";
    case HttpStatusCode::kBadRequest:
      return "Bad Request";
    case HttpStatusCode::kUnauthorized:
      return "Authorization Required";
    case HttpStatusCode::kPaymentRequired:
      return "Payment Required";
    case HttpStatusCode::kForbidden:
      return "Forbidden";
    case HttpStatusCode::kNotFound:
      return "Not Found";
    case HttpStatusCode::kMethodNotAllowed:
      return "Method Not Allowed";
    case HttpStatusCode::kNotAcceptable:
      return "Not Acceptable";
    case HttpStatusCode::kRequestTimeout:
      return "Request Time-out";
    case HttpStatusCode::kLengthRequired:
      return "Length Required";
    case HttpStatusCode::kPreconditionFailed:
      return "Precondition Failed";
    case HttpStatusCode::kRequestEntityTooLarge:
      return "Request Entity Too Large";
    case HttpStatusCode::kRequestUriTooLarge:
      return "Request-URI Too Large";
    case HttpStatusCode::kUnsupportedMediaType:
      return "Unsupported Media Type";
    case HttpStatusCode::kRangeNotSatisfiable:
      return "Request Range Not Satisfiable";
    case HttpStatusCode::kExpectationFailed:
      return "Expectation Failed";
    case HttpStatusCode::kUnprocessableEntity:
      return "Unprocessable Entity";
    case HttpStatusCode::kLocked:
      return "Locked";
    case HttpStatusCode::kFailedDependency:
      return "Failed Dependency";
    case HttpStatusCode::kUpgradeRequired:
      return "Upgrade Required";
    case HttpStatusCode::kUnavailableForLegalReasons:
      return "Unavailable For Legal Reasons";
    case HttpStatusCode::kInternalServerError:
      return "Internal Error";
    case HttpStatusCode::kNotImplemented:
      return "Method Not Implemented";
    case HttpStatusCode::kBadGateway:
      return "Bad Gateway";
    case HttpStatusCode::kServiceUnavailable:
      return "Service Temporarily Unavailable";
    case HttpStatusCode::kGatewayTimeout:
      return "Gateway Time-out";
    case HttpStatusCode::kVersionNotSupported:
      return "HTTP Version Not Supported";
    case HttpStatusCode::kVariantAlsoVaries:
      return "Variant Also Negotiates";
    case HttpStatusCode::kInsufficientStorage:
      return "Insufficent Storage";
    case HttpStatusCode::kNotExtended:
      return "Not Extended";
    case HttpStatusCode::kFrequencyCapping:
      return "Frequency Capped";
    default:
      return "Unknown Status Code";
    }
  }

  inline HttpStatusCode ToHttpStatusCode(std::string_view status) noexcept {
    int value{};
    auto [ptr, ec]{std::from_chars(status.data(), status.data() + status.size(), value)};
    if (ec != std::errc()) {
      return HttpStatusCode::kUnknown;
    }

    return magic_enum::enum_cast<HttpStatusCode>(value).value_or(HttpStatusCode::kUnknown);
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
