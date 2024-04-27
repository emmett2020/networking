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

#include "utils/execution.h"
#include "http/v1/http1_request.h"
#include "http/v1/http1_response.h"

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

  using http_handler = std::function<void(const http_request&, http_response&)>;

  // Handle http request then request response.
  inline ex::sender auto handle_request(const http_request& request) noexcept {
    http_response response{};
    response.status_code = http_status_code::ok;
    response.version = request.version;
    response.headers = request.headers;
    response.body = request.body;
    response.need_keepalive = need_keepalive(request);
    return ex::just(std::move(response));
  }

} // namespace net::http::http1
