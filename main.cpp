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

#include <fmt/format.h>

#include "net/http.h"

namespace http = net::http;

int main() {
  exec::io_uring_context context;
  auto addr = sio::ip::address_v4::any();
  constexpr net::http::port_t port = 8080;
  http::server server{context, addr, port};

  server.register_handler(
    http::http_method::get | http::http_method::post,
    "/echo",
    [](http::http_connection& conn) {
      const http::http_request& req = conn.request;
      http::http_response& rsp = conn.response;
      rsp.version = req.version;
      rsp.status_code = http::http_status_code::ok;
      rsp.headers = req.headers;
      rsp.body = req.body;
    });

  fmt::println("start listening on {}:{}", addr.to_string(), port);
  net::http::start_server(server);
  return 0;
}
