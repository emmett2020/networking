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

namespace net::http::common {
  namespace detail {
    static constexpr std::array<std::string_view, 5> http_versions =
      {"HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/3.0", "UNKNOWN_HTTP_VERSION"};

  }

  /**
   * @detail HTTP scheme
  */
  enum class http_scheme {
    http,
    https,
    unknown
  };

  /**
   * @detail HTTP version
  */
  enum class http_version {
    http10,
    http11,
    http20,
    http30,
    unknown
  };

  /**
   * @detail Convert given integer number to http version enum.
   * @param total Equals to http major version * 10 + http minor version
  */
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

  /// @detail Convert given integer numbers to http version enum.
  constexpr http_version to_http_version(int major, int minor) {
    return to_http_version(major * 10 + minor);
  }

  /// @detail Convert given HTTP version enum to uppercase string.
  constexpr std::string_view to_http_version_string(http_version version) noexcept {
    if (version == http_version::http10) {
      return detail::http_versions[0];
    }
    if (version == http_version::http11) {
      return detail::http_versions[1];
    }
    if (version == http_version::http20) {
      return detail::http_versions[2];
    }
    if (version == http_version::http30) {
      return detail::http_versions[3];
    }
    return detail::http_versions[4];
  }

  /**
   * @brief HTTP method definition.
   * @attention Note that the delete method is not named "delete" because it conflicts with
   * the delete keyword, we name it "del".
  */
  enum class http_method {
    get,
    head,
    post,
    put,
    del,
    trace,
    control,
    purge,
    options,
    connect,
    unknown
  };

  /// @brief Convert given HTTP method enum to uppercase string.
  constexpr std::string_view to_http_method_string(http_method method) noexcept {
    switch (method) {
    case http_method::get:
      return "GET";
    case http_method::head:
      return "HEAD";
    case http_method::post:
      return "POST";
    case http_method::put:
      return "PUT";
    case http_method::del:
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

  /// @brief Convert given HTTP method string to enum.
  constexpr http_method to_http_method(std::string_view method) noexcept {
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
      return http_method::del;
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

  /**
   * @brief HTTP status code definition.
   * @attention Note that the continue status code is not named "continue" because it conflicts with
   * the continue keyword, we name it "cont".
  */
  enum class http_status_code {
    unknown = 0,
    cont = 100,
    ok = 200,
    create = 201,
    accepted = 202,
    non_authoritative = 203,
    no_content = 204,
    reset_content = 205,
    partial_content = 206,
    multi_status = 207,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    see_other = 303,
    not_modified = 304,
    use_proxy = 305,
    temporary_redirect = 307,
    permanent_redirect = 308,
    bad_request = 400,
    unauthorized = 401,
    payment_required = 402,
    forbidden = 403,
    not_found = 404,
    method_not_allowed = 405,
    not_acceptable = 406,
    request_timeout = 408,
    length_required = 411,
    precondition_failed = 412,
    request_entity_too_large = 413,
    request_uri_too_large = 414,
    unsupported_media_type = 415,
    range_not_satisfiable = 416,
    expectation_failed = 417,
    unprocessable_entity = 422,
    locked = 423,
    failed_dependency = 424,
    upgrade_required = 426,
    unavailable_for_legal_reasons = 451,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503,
    gateway_timeout = 504,
    version_not_supported = 505,
    variant_also_varies = 506,
    insufficient_storage = 507,
    not_extended = 510,
    frequency_capping = 514,
    script_server_error = 544,
  };

  /// @brief Convert given HTTP status code to integer string.
  constexpr std::string to_http_status_code_string(http_status_code code) noexcept {
    switch (code) {
    case http_status_code::unknown:
      return "0";
    case http_status_code::cont:
      return "100";
    case http_status_code::ok:
      return "200";
    case http_status_code::create:
      return "201";
    case http_status_code::accepted:
      return "202";
    case http_status_code::non_authoritative:
      return "203";
    case http_status_code::no_content:
      return "204";
    case http_status_code::reset_content:
      return "205";
    case http_status_code::partial_content:
      return "206";
    case http_status_code::multi_status:
      return "207";
    case http_status_code::multiple_choices:
      return "300";
    case http_status_code::moved_permanently:
      return "301";
    case http_status_code::moved_temporarily:
      return "302";
    case http_status_code::see_other:
      return "303";
    case http_status_code::not_modified:
      return "304";
    case http_status_code::use_proxy:
      return "305";
    case http_status_code::temporary_redirect:
      return "307";
    case http_status_code::permanent_redirect:
      return "308";
    case http_status_code::bad_request:
      return "400";
    case http_status_code::unauthorized:
      return "401";
    case http_status_code::payment_required:
      return "402";
    case http_status_code::forbidden:
      return "403";
    case http_status_code::not_found:
      return "404";
    case http_status_code::method_not_allowed:
      return "405";
    case http_status_code::not_acceptable:
      return "406";
    case http_status_code::request_timeout:
      return "408";
    case http_status_code::length_required:
      return "411";
    case http_status_code::precondition_failed:
      return "412";
    case http_status_code::request_entity_too_large:
      return "413";
    case http_status_code::request_uri_too_large:
      return "414";
    case http_status_code::unsupported_media_type:
      return "415";
    case http_status_code::range_not_satisfiable:
      return "416";
    case http_status_code::expectation_failed:
      return "417";
    case http_status_code::unprocessable_entity:
      return "422";
    case http_status_code::locked:
      return "423";
    case http_status_code::failed_dependency:
      return "424";
    case http_status_code::upgrade_required:
      return "426";
    case http_status_code::unavailable_for_legal_reasons:
      return "451";
    case http_status_code::internal_server_error:
      return "500";
    case http_status_code::not_implemented:
      return "501";
    case http_status_code::bad_gateway:
      return "502";
    case http_status_code::service_unavailable:
      return "503";
    case http_status_code::gateway_timeout:
      return "504";
    case http_status_code::version_not_supported:
      return "505";
    case http_status_code::variant_also_varies:
      return "506";
    case http_status_code::insufficient_storage:
      return "507";
    case http_status_code::not_extended:
      return "510";
    case http_status_code::frequency_capping:
      return "514";
    case http_status_code::script_server_error:
      return "544";
    default:
      return "0";
    }
  }

  /// @brief Convert given HTTP status code to string.
  constexpr std::string_view to_http_status_reason(http_status_code code) noexcept {
    switch (code) {
    case http_status_code::unknown:
      return "Unknown Status";
    case http_status_code::cont:
      return "Continue";
    case http_status_code::ok:
      return "OK";
    case http_status_code::create:
      return "Created";
    case http_status_code::accepted:
      return "Accepted";
    case http_status_code::non_authoritative:
      return "Non-Authoritative Information";
    case http_status_code::no_content:
      return "No Content";
    case http_status_code::reset_content:
      return "Reset Content";
    case http_status_code::partial_content:
      return "Partial Content";
    case http_status_code::multi_status:
      return "Multi-Status";
    case http_status_code::multiple_choices:
      return "Multiple Choices";
    case http_status_code::moved_permanently:
      return "Moved Permanently";
    case http_status_code::moved_temporarily:
      return "Found";
    case http_status_code::see_other:
      return "See Other";
    case http_status_code::not_modified:
      return "Not Modified";
    case http_status_code::use_proxy:
      return "Use Proxy";
    case http_status_code::temporary_redirect:
      return "Temporary Redirect";
    case http_status_code::permanent_redirect:
      return "Permanent Redirect";
    case http_status_code::bad_request:
      return "Bad Request";
    case http_status_code::unauthorized:
      return "Authorization Required";
    case http_status_code::payment_required:
      return "Payment Required";
    case http_status_code::forbidden:
      return "Forbidden";
    case http_status_code::not_found:
      return "Not Found";
    case http_status_code::method_not_allowed:
      return "Method Not Allowed";
    case http_status_code::not_acceptable:
      return "Not Acceptable";
    case http_status_code::request_timeout:
      return "Request Time-out";
    case http_status_code::length_required:
      return "Length Required";
    case http_status_code::precondition_failed:
      return "Precondition Failed";
    case http_status_code::request_entity_too_large:
      return "Request Entity Too Large";
    case http_status_code::request_uri_too_large:
      return "Request-URI Too Large";
    case http_status_code::unsupported_media_type:
      return "Unsupported Media Type";
    case http_status_code::range_not_satisfiable:
      return "Request Range Not Satisfiable";
    case http_status_code::expectation_failed:
      return "Expectation Failed";
    case http_status_code::unprocessable_entity:
      return "Unprocessable Entity";
    case http_status_code::locked:
      return "Locked";
    case http_status_code::failed_dependency:
      return "Failed Dependency";
    case http_status_code::upgrade_required:
      return "Upgrade Required";
    case http_status_code::unavailable_for_legal_reasons:
      return "Unavailable For Legal Reasons";
    case http_status_code::internal_server_error:
      return "Internal Error";
    case http_status_code::not_implemented:
      return "Method Not Implemented";
    case http_status_code::bad_gateway:
      return "Bad Gateway";
    case http_status_code::service_unavailable:
      return "Service Temporarily Unavailable";
    case http_status_code::gateway_timeout:
      return "Gateway Time-out";
    case http_status_code::version_not_supported:
      return "HTTP Version Not Supported";
    case http_status_code::variant_also_varies:
      return "Variant Also Negotiates";
    case http_status_code::insufficient_storage:
      return "Insufficent Storage";
    case http_status_code::not_extended:
      return "Not Extended";
    case http_status_code::frequency_capping:
      return "Frequency Capped";
    default:
      return "Unknown Status Code";
    }
  }

  /// @brief Convert given string to http status code enum.
  constexpr http_status_code to_http_status_code(std::string_view status) noexcept {
    int value = 0;
    auto [ptr, ec] = std::from_chars(status.data(), status.data() + status.size(), value);
    if (ec != std::errc()) {
      return http_status_code::unknown;
    }
    return magic_enum::enum_cast<http_status_code>(value).value_or(http_status_code::unknown);
  }

  static constexpr std::string_view http_header_host = "Host";
  static constexpr std::string_view http_header_content_length = "Content-Length";
  static constexpr std::string_view http_header_if_modified_since = "If-Modified-Since";
  static constexpr std::string_view http_header_etag = "Etag";
  static constexpr std::string_view http_header_accept_encoding = "Accept-Encoding";
  static constexpr std::string_view http_header_last_modified = "Last-Modified";
  static constexpr std::string_view http_header_content_range = "Content-Range";
  static constexpr std::string_view http_header_content_type = "Content-Type";
  static constexpr std::string_view http_header_transfer_encoding = "Transfer-Encoding";
  static constexpr std::string_view http_header_content_encoding = "Content-Encoding";
  static constexpr std::string_view http_header_connection = "Connection";
  static constexpr std::string_view http_header_range = "Range";
  static constexpr std::string_view http_header_server = "Server";
  static constexpr std::string_view http_header_date = "Date";
  static constexpr std::string_view http_header_location = "Location";
  static constexpr std::string_view http_header_expect = "Expect";
  static constexpr std::string_view http_header_cache_control = "Cache-Control";
  static constexpr std::string_view http_header_cache_tag = "Cache-Tag";
  static constexpr std::string_view http_header_expires = "Expires";
  static constexpr std::string_view http_header_referer = "Referer";
  static constexpr std::string_view http_header_user_agent = "User-Agent";
  static constexpr std::string_view http_header_cookie = "Cookie";
  static constexpr std::string_view http_header_x_forwarded_for = "X-Forwarded-For";
  static constexpr std::string_view http_header_accept_language = "Accept-Language";
  static constexpr std::string_view http_header_accept_charset = "Accept-Charset";
  static constexpr std::string_view http_header_accept_ranges = "Accept-Ranges";
  static constexpr std::string_view http_header_set_cookie = "Set-Cookie";
  static constexpr std::string_view http_header_via = "Via";
  static constexpr std::string_view http_header_pragma = "Pragma";
  static constexpr std::string_view http_header_upgrade = "Upgrade";
  static constexpr std::string_view http_header_if_none_match = "If-None-Match";
  static constexpr std::string_view http_header_if_match = "If-Match";
  static constexpr std::string_view http_header_if_range = "If-Range";
  static constexpr std::string_view http_header_accept = "Accept";
  static constexpr std::string_view http_header_age = "Age";
  static constexpr std::string_view http_header_chunked = "chunked";
  static constexpr std::string_view http_header_identity = "identity";
  static constexpr std::string_view http_header_keepalive = "keep-alive";
  static constexpr std::string_view http_header_close = "close";

}; // namespace net::http1
