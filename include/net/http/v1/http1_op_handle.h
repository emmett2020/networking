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

#include <stdexec/execution.hpp>
#include <utility>

#include "net/http/http_error.h"
#include "net/http/http_server.h"
#include "net/http/http_common.h"
#include "net/http/http_request.h"

namespace net::http::http1 {
  namespace ex = stdexec;

  inline bool need_keepalive(const http_request& request) noexcept {
    if (request.headers.contains(header::connection)) {
      return true;
    }
    if (request.version == http_version::http11) {
      return true;
    }
    return false;
  }

  inline bool matches(const std::string& url, const handler_pattern& pattern) {
    return pattern.url_pattern == url;
  }

  using handle_request_t = decltype(ex::just(std::declval<http_connection>()));

  // Handle http request then request response.
  inline handle_request_t handle_request(http_connection& conn) {
    const http_request& request = conn.request;
    conn.need_keepalive = need_keepalive(request);

    auto method_idx = magic_enum::enum_index(request.method);
    if (!method_idx) {
      throw http_error("unknown request method");
    }

    const auto& handlers = conn.serv->handlers[*method_idx];
    if (handlers.empty()) {
      throw http_error("empty handler");
    }

    // Find the most exactly matches pattern.
    int idx = -1;
    auto handler_size = static_cast<int>(handlers.size());
    for (int i = 0; i < handler_size; ++i) {
      if (matches(request.path, handlers[i])) {
        idx = i;
      }
    }
    if (idx == -1) {
      throw http_error("not found suitable handler");
    }
    const auto& pattern = handlers[idx];

    // Call user registered callback function.
    pattern.handler(conn);
    return ex::just(std::move(conn));
  }

} // namespace net::http::http1
