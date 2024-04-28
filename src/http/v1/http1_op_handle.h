/*
 * Copyright (c) 2024 Xiaoming Zhang
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
#include "utils/execution.h"
#include "http/v1/http_connection.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace net::http::http1 {
  inline bool need_keepalive(const http_request& request) noexcept {
    if (request.headers.contains(http_header_connection)) {
      return true;
    }
    if (request.version == http_version::http11) {
      return true;
    }
    return false;
  }

  using http_handler = std::function<void(std::string_view, const http_request&, http_response&)>;

  // Handle http request then request response.
  inline ex::sender auto handle_request(http_connection& conn) noexcept {
    const http_request& request = conn.request;
    http_response& response = conn.response;
    response.status_code = http_status_code::ok;
    response.version = request.version;
    response.headers = request.headers;
    response.body = request.body;
    response.need_keepalive = need_keepalive(request);
    return ex::just(std::move(conn));
  }

} // namespace net::http::http1
