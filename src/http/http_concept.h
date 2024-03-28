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
#include <functional>
#include <unordered_map>
#include <utility>

#include "http/http_common.h"
#include "http/hashtable.h"

namespace net::http::http1 {

  namespace detail {
    template <typename T>
    concept _http1_header_concept = requires(T& t) {
      //
      { t.headers() }; // TODO: return what?
      { t.insert_header(std::string{}, std::string{}) } -> std::convertible_to<bool>;
      { t.delete_header(std::string{}) } -> std::same_as<void>;
      { t.update_header(std::string{}, std::string{}) } -> std::same_as<void>;
      { t.clear_headers() } -> std::same_as<void>;
      { t.contain_header(std::string{}) } -> std::convertible_to<bool>;
      { t.append_header_value(std::string{}, std::string{}) } -> std::same_as<bool>;
      { t.count_header() } -> std::convertible_to<std::uint32_t>;
      {
        t.header_value(std::string{})
      } -> std::convertible_to<std::optional<std::reference_wrapper<std::string>>>;
    };
  } // namespace detail

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
  concept simple_hashtable_concept = requires(T& t) {
    // type definition
    { T::key_type };
    { T::mapped_type };

    // iterator
    { t.begin() };
    { t.end() };

    // operations
    { t.insert() };
    { t.remove() };
    { t.update() };
    { t.contain() };
    { t.get_value() };
    { t.append_value() };
  };

  template <typename T>
  concept http1_request_concept = requires(T& t) {
    { t.method } -> std::convertible_to<http_method>;
    { t.scheme } -> std::same_as<http_scheme>;
    { t.uri } -> std::same_as<std::string>;
    { t.host } -> std::same_as<std::string>;
    { t.port } -> std::convertible_to<uint16_t>;
    { t.path } -> std::same_as<std::string>;
    { t.version } -> std::same_as<http_version>;
    { t.body } -> std::same_as<std::string>;
    { t.content_length } -> std::convertible_to<std::size_t>;
    { simple_hashtable_concept<decltype(t.headers)> };
    { simple_hashtable_concept<decltype(t.params)> };
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
