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

#include <array>
#include <charconv>
#include <string>
#include <string_view>

#include <magic_enum.hpp>

namespace net::http {

enum class Protocol {
  kProtocolHttp,
  kProtocolHttps,
  kProtocolQuic,
  kProtocolUnknown
};

static constexpr std::array<std::string_view, 5> kHttpVersions = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/3", "UNKNOWN_VERSION"};

enum class HttpMethod {
  kHttpMethodGet,
  kHttpMethodHead,
  kHttpMethodPost,
  kHttpMethodPut,
  kHttpMethodDelete,
  kHttpMethodTrace,
  kHttpMethodControl,
  kHttpMethodPurge,
  kHttpMethodOptions,
  kHttpMethodUnknown
};

constexpr std::string_view HttpMethodToString(HttpMethod method) noexcept {
  switch (method) {
    case HttpMethod::kHttpMethodGet:
      return "GET";
    case HttpMethod::kHttpMethodHead:
      return "HEAD";
    case HttpMethod::kHttpMethodPost:
      return "POST";
    case HttpMethod::kHttpMethodPut:
      return "PUT";
    case HttpMethod::kHttpMethodDelete:
      return "DELETE";
    case HttpMethod::kHttpMethodTrace:
      return "TRACE";
    case HttpMethod::kHttpMethodControl:
      return "CONTROL";
    case HttpMethod::kHttpMethodPurge:
      return "PURGE";
    case HttpMethod::kHttpMethodOptions:
      return "OPTIONS";
    default:
      return "UNKNOWN";
  }
}

constexpr HttpMethod StringToHttpMethod(std::string_view method) noexcept {
  if (method == "GET") {
    return HttpMethod::kHttpMethodGet;
  }
  if (method == "HEAD") {
    return HttpMethod::kHttpMethodHead;
  }
  if (method == "POST") {
    return HttpMethod::kHttpMethodPost;
  }
  if (method == "PUT") {
    return HttpMethod::kHttpMethodPut;
  }
  if (method == "DELETE") {
    return HttpMethod::kHttpMethodDelete;
  }
  if (method == "TRACE") {
    return HttpMethod::kHttpMethodTrace;
  }
  if (method == "CONTROL") {
    return HttpMethod::kHttpMethodControl;
  }
  if (method == "PURGE") {
    return HttpMethod::kHttpMethodPurge;
  }
  if (method == "OPTIONS") {
    return HttpMethod::kHttpMethodOptions;
  }

  return HttpMethod::kHttpMethodUnknown;
}

enum class HttpStatusCode {
  kHttpStatusCodeUnknown = 0,
  kHttpStatusCodeContinue = 100,
  kHttpStatusCodeOK = 200,
  kHttpStatusCodeCreate = 201,
  kHttpStatusCodeAccepted = 202,
  kHttpStatusCodeNonAuthoritative = 203,
  kHttpStatusCodeNoContent = 204,
  kHttpStatusCodeResetContent = 205,
  kHttpStatusCodePartialContent = 206,
  kHttpStatusCodeMultiStatus = 207,
  kHttpStatusCodeMultipleChoices = 300,
  kHttpStatusCodeMovedPermanently = 301,
  kHttpStatusCodeMovedTemporarily = 302,
  kHttpStatusCodeSeeOther = 303,
  kHttpStatusCodeNotModified = 304,
  kHttpStatusCodeUseProxy = 305,
  kHttpStatusCodeTemporaryRedirect = 307,
  kHttpStatusCodePermanentRedirect = 308,
  kHttpStatusCodeBadRequest = 400,
  kHttpStatusCodeUnauthorized = 401,
  kHttpStatusCodePaymentRequired = 402,
  kHttpStatusCodeForbidden = 403,
  kHttpStatusCodeNotFound = 404,
  kHttpStatusCodeMethodNotAllowed = 405,
  kHttpStatusCodeNotAcceptable = 406,
  kHttpStatusCodeRequestTimeout = 408,
  kHttpStatusCodeLengthRequired = 411,
  kHttpStatusCodePreconditionFailed = 412,
  kHttpStatusCodeRequestEntityTooLarge = 413,
  kHttpStatusCodeRequestUriTooLarge = 414,
  kHttpStatusCodeUnsupportedMediaType = 415,
  kHttpStatusCodeRangeNotSatisfiable = 416,
  kHttpStatusCodeExpectationFailed = 417,
  kHttpStatusCodeUnprocessableEntity = 422,
  kHttpStatusCodeLocked = 423,
  kHttpStatusCodeFailedDependency = 424,
  kHttpStatusCodeUpgradeRequired = 426,
  kHttpStatusCodeUnavailableForLegalReasons = 451,
  kHttpStatusCodeInternalServerError = 500,
  kHttpStatusCodeNotImplemented = 501,
  kHttpStatusCodeBadGateway = 502,
  kHttpStatusCodeServiceUnavailable = 503,
  kHttpStatusCodeGatewayTimeout = 504,
  kHttpStatusCodeVersionNotSupported = 505,
  kHttpStatusCodeVariantAlsoVaries = 506,
  kHttpStatusCodeInsufficientStorage = 507,
  kHttpStatusCodeNotExtended = 510,
  kHttpStatusCodeFrequencyCapping = 514,
  kHttpStatusCodeScriptServerError = 544,
};

constexpr std::string_view HttpStatusCodeToString(
    HttpStatusCode code) noexcept {
  switch (code) {
    case HttpStatusCode::kHttpStatusCodeUnknown:
      return "Unknown Status";
    case HttpStatusCode::kHttpStatusCodeContinue:
      return "Continue";
    case HttpStatusCode::kHttpStatusCodeOK:
      return "OK";
    case HttpStatusCode::kHttpStatusCodeCreate:
      return "Created";
    case HttpStatusCode::kHttpStatusCodeAccepted:
      return "Accepted";
    case HttpStatusCode::kHttpStatusCodeNonAuthoritative:
      return "Non-Authoritative Information";
    case HttpStatusCode::kHttpStatusCodeNoContent:
      return "No Content";
    case HttpStatusCode::kHttpStatusCodeResetContent:
      return "Reset Content";
    case HttpStatusCode::kHttpStatusCodePartialContent:
      return "Partial Content";
    case HttpStatusCode::kHttpStatusCodeMultiStatus:
      return "Multi-Status";
    case HttpStatusCode::kHttpStatusCodeMultipleChoices:
      return "Multiple Choices";
    case HttpStatusCode::kHttpStatusCodeMovedPermanently:
      return "Moved Permanently";
    case HttpStatusCode::kHttpStatusCodeMovedTemporarily:
      return "Found";
    case HttpStatusCode::kHttpStatusCodeSeeOther:
      return "See Other";
    case HttpStatusCode::kHttpStatusCodeNotModified:
      return "Not Modified";
    case HttpStatusCode::kHttpStatusCodeUseProxy:
      return "Use Proxy";
    case HttpStatusCode::kHttpStatusCodeTemporaryRedirect:
      return "Temporary Redirect";
    case HttpStatusCode::kHttpStatusCodePermanentRedirect:
      return "Permanent Redirect";
    case HttpStatusCode::kHttpStatusCodeBadRequest:
      return "Bad Request";
    case HttpStatusCode::kHttpStatusCodeUnauthorized:
      return "Authorization Required";
    case HttpStatusCode::kHttpStatusCodePaymentRequired:
      return "Payment Required";
    case HttpStatusCode::kHttpStatusCodeForbidden:
      return "Forbidden";
    case HttpStatusCode::kHttpStatusCodeNotFound:
      return "Not Found";
    case HttpStatusCode::kHttpStatusCodeMethodNotAllowed:
      return "Method Not Allowed";
    case HttpStatusCode::kHttpStatusCodeNotAcceptable:
      return "Not Acceptable";
    case HttpStatusCode::kHttpStatusCodeRequestTimeout:
      return "Request Time-out";
    case HttpStatusCode::kHttpStatusCodeLengthRequired:
      return "Length Required";
    case HttpStatusCode::kHttpStatusCodePreconditionFailed:
      return "Precondition Failed";
    case HttpStatusCode::kHttpStatusCodeRequestEntityTooLarge:
      return "Request Entity Too Large";
    case HttpStatusCode::kHttpStatusCodeRequestUriTooLarge:
      return "Request-URI Too Large";
    case HttpStatusCode::kHttpStatusCodeUnsupportedMediaType:
      return "Unsupported Media Type";
    case HttpStatusCode::kHttpStatusCodeRangeNotSatisfiable:
      return "Request Range Not Satisfiable";
    case HttpStatusCode::kHttpStatusCodeExpectationFailed:
      return "Expectation Failed";
    case HttpStatusCode::kHttpStatusCodeUnprocessableEntity:
      return "Unprocessable Entity";
    case HttpStatusCode::kHttpStatusCodeLocked:
      return "Locked";
    case HttpStatusCode::kHttpStatusCodeFailedDependency:
      return "Failed Dependency";
    case HttpStatusCode::kHttpStatusCodeUpgradeRequired:
      return "Upgrade Required";
    case HttpStatusCode::kHttpStatusCodeUnavailableForLegalReasons:
      return "Unavailable For Legal Reasons";
    case HttpStatusCode::kHttpStatusCodeInternalServerError:
      return "Internal Error";
    case HttpStatusCode::kHttpStatusCodeNotImplemented:
      return "Method Not Implemented";
    case HttpStatusCode::kHttpStatusCodeBadGateway:
      return "Bad Gateway";
    case HttpStatusCode::kHttpStatusCodeServiceUnavailable:
      return "Service Temporarily Unavailable";
    case HttpStatusCode::kHttpStatusCodeGatewayTimeout:
      return "Gateway Time-out";
    case HttpStatusCode::kHttpStatusCodeVersionNotSupported:
      return "HTTP Version Not Supported";
    case HttpStatusCode::kHttpStatusCodeVariantAlsoVaries:
      return "Variant Also Negotiates";
    case HttpStatusCode::kHttpStatusCodeInsufficientStorage:
      return "Insufficent Storage";
    case HttpStatusCode::kHttpStatusCodeNotExtended:
      return "Not Extended";
    case HttpStatusCode::kHttpStatusCodeFrequencyCapping:
      return "Frequency Capped";
    default:
      return "Unknown Status";
  }
}

constexpr HttpStatusCode StringToHttpStatusCode(
    std::string_view status) noexcept {
  int value{};
  auto [ptr, ec]{
      std::from_chars(status.data(), status.data() + status.size(), value)};
  if (ec != std::errc()) {
    return HttpStatusCode::kHttpStatusCodeUnknown;
  }

  return magic_enum::enum_cast<HttpStatusCode>(value).value_or(
      HttpStatusCode::kHttpStatusCodeUnknown);
}

static constexpr std::string_view kHttpHeaderHost = "Host";
static constexpr std::string_view kHttpHeaderContentLength = "Content-Length";
static constexpr std::string_view kHttpHeaderIfModifiedSince =
    "If-Modified-Since";
static constexpr std::string_view kHttpHeaderEtag = "Etag";
static constexpr std::string_view kHttpHeaderAcceptEncoding = "Accept-Encoding";
static constexpr std::string_view kHttpHeaderLastModified = "Last-Modified";
static constexpr std::string_view kHttpHeaderContentRange = "Content-Range";
static constexpr std::string_view kHttpHeaderContentType = "Content-Type";
static constexpr std::string_view kHttpHeaderTransferEncoding =
    "Transfer-Encoding";
static constexpr std::string_view kHttpHeaderContentEncoding =
    "Content-Encoding";
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

};  // namespace net::http
