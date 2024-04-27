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

namespace net::http {
  template <typename...>
  concept always_true_implement_later = true;

  template <typename T>
  concept http1_headers = always_true_implement_later<T>;

  template <typename T>
  concept http1_parameters = always_true_implement_later<T>;

  template <typename T>
  concept http1_metric_concept = always_true_implement_later<T>;

  template <typename T>
  concept http1_option_concept = always_true_implement_later<T>;

  template <typename T>
  concept http1_request_concept = requires(T& t) {
    { std::convertible_to<decltype(t.uri), std::string> };
    { std::convertible_to<decltype(t.host), std::string> };
    { std::convertible_to<decltype(t.path), std::string> };
    { std::convertible_to<decltype(t.body), std::string> };
    { std::convertible_to<decltype(t.method), http_method> };
    { std::convertible_to<decltype(t.scheme), http_scheme> };
    { std::convertible_to<decltype(t.port), std::size_t> };
    { std::convertible_to<decltype(t.content_length), std::size_t> };
    { http1_headers<decltype(t.headers)> };
    { http1_parameters<decltype(t.params)> };
  };

  template <typename T>
  concept http1_response_concept = requires(T& t) {
    { std::convertible_to<decltype(t.version), http_version> };
    { std::convertible_to<decltype(t.status_code), http_status_code > };
    { std::convertible_to<decltype(t.reason), std::string> };
    { std::convertible_to<decltype(t.content_length), std::size_t> };
    { http1_headers<decltype(t.headers)> };
  };

  template <typename T>
  concept http1_message_concept = http1_response_concept<T> || http1_request_concept<T>;

  template <typename T>
  concept http1_server_concept = always_true_implement_later<T>;

  template <typename T>
  concept http1_socket_concept = always_true_implement_later<T>;

  template <typename T>
  concept socket_buffer_concept = always_true_implement_later<T>;

} // namespace net::http
