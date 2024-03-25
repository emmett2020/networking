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

  template <typename T>
  concept http1_request_concept = requires(T& t) {
    { t.method() } -> std::convertible_to<http_method>;
    { t.scheme() } -> std::convertible_to<http_scheme>;
    { t.version() } -> std::convertible_to<http_version>;
    { t.port() } -> std::convertible_to<uint16_t>;
    { t.host() } -> std::convertible_to<std::string>;
    { t.path() } -> std::convertible_to<std::string>;
    { t.uri() } -> std::convertible_to<std::string>;
    { t.body() } -> std::convertible_to<std::string>;
    { t.content_length() } -> std::convertible_to<std::size_t>;
    { t.headers() } -> std::convertible_to<std::unordered_map<std::string, std::string>>;
    { t.params() } -> std::convertible_to<std::unordered_map<std::string, std::string>>;
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
