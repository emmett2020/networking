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

#include <concepts>
#include <cstdint>
#include <utility>
#include "http/http_common.h"

namespace net::http::http1 {
  template <typename T>
  concept http1_response_concept = requires(T& t) {
    { t.version() } -> std::convertible_to<http_version>;
    { t.status_code() } -> std::convertible_to<http_status_code>;
    { t.reason() } -> std::convertible_to<std::string>;
    { t.body() } -> std::convertible_to<std::string>;
    { t.content_length() } -> std::convertible_to<std::size_t>;
    { t.headers() } -> std::convertible_to<std::unordered_map<std::string, std::string>>;
  };

  // TODO: TOO MANY
  template <typename T>
  concept http1_request_concept = requires(T& t, const T& ct) {
    { t.method() } -> std::same_as<http_method>;
    { t.set_method(std::declval<http_method>()) } -> std::same_as<void>;

    { t.scheme() } -> std::same_as<http_scheme>;
    { t.set_scheme(std::declval<http_scheme>()) } -> std::same_as<void>;

    { t.version() } -> std::same_as<http_version>;
    { t.set_version(std::declval<http_version>()) } -> std::same_as<void>;

    { t.port() } -> std::convertible_to<uint16_t>;
    { t.set_port(std::uint16_t{}) } -> std::same_as<void>;

    { t.host() } -> std::convertible_to<std::string&>;
    { ct.host() } -> std::convertible_to<const std::string&>;
    { t.set_host(std::string{}) } -> std::same_as<void>;

    { t.path() } -> std::convertible_to<std::string&>;
    { ct.path() } -> std::convertible_to<const std::string&>;
    { t.set_path(std::string{}) } -> std::same_as<void>;

    { t.uri() } -> std::convertible_to<std::string&>;
    { ct.uri() } -> std::convertible_to<const std::string&>;
    { t.set_uri() } -> std::same_as<void>;

    { t.body() } -> std::convertible_to<std::string&>;
    { ct.body() } -> std::convertible_to<const std::string&>;
    { t.set_body() } -> std::same_as<void>;

    { t.content_length() } -> std::convertible_to<std::size_t>;
    { t.set_content_length(std::size_t{}) } -> std::same_as<void>;

    { t.headers() };
    { t.add_header(std::string{}) } -> std::convertible_to<bool>;
    { t.del_header(std::string{}) } -> std::convertible_to<bool>;
    { t.clear_header() } -> std::same_as<void>;
    { t.contain_header(std::string{}) } -> std::convertible_to<bool>;
    { t.append_to_header(std::string{}, std::string{}) } -> std::convertible_to<bool>;
    { t.header_count() } -> std::convertible_to<std::uint32_t>;

    { t.params() };
    { t.add_param(std::string{}) } -> std::convertible_to<bool>;
    { t.del_param(std::string{}) } -> std::convertible_to<bool>;
    { t.clear_param() } -> std::same_as<void>;
    { t.contain_param(std::string{}) } -> std::convertible_to<bool>;
    { t.append_to_param(std::string{}, std::string{}) } -> std::convertible_to<bool>;
    { t.param_count() } -> std::convertible_to<std::uint32_t>;
  };

  template <typename T>
  concept http1_message_concept = http1_response_concept<T> || http1_request_concept<T>;

  template <typename T>
  concept http1_server_concept = true;

  template <typename T>
  concept http1_socket_concept = true;

  template <typename T>
  concept socket_buffer_concept = true;

  template <typename T>
  concept http1_metric_concept = true;

  template <typename T>
  concept http1_option_concept = true;
} // namespace net::http::http1
