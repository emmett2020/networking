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

#include <fmt/format.h>
#include <stdexec/execution.hpp>
#include <exec/linux/io_uring_context.hpp>

#include "http/http1.h"
#include "http/http_common.h"
#include "http/http_response.h"
#include "http/v1/http_connection.h"

using namespace net; // NOLINT

int main() {
  constexpr std::string_view ip = "127.0.0.1";
  constexpr net::http::port_t port = 8080;
  fmt::println("start listening on {}:{}", ip, port);
  ex::io_uring_context context;
  net::http::server server{context, ip, port};

  server.register_handler(
    http::http_method::get | http::http_method::post,
    "/echo",
    [](http::http1::http_connection& conn) {
      const http::http_request& req = conn.request;
      http::http_response& rsp = conn.response;
      rsp.version = req.version;
      rsp.status_code = http::http_status_code::ok;
      rsp.headers = req.headers;
      rsp.body = req.body;
    });

  server.register_handler(http::all_methods, "*", [](http::http1::http_connection& conn) {
    const http::http_request& req = conn.request;
    http::http_response& rsp = conn.response;
    rsp.version = req.version;
    rsp.status_code = http::http_status_code::not_found;
  });


  net::http::start_server(server);
  return 0;
}
