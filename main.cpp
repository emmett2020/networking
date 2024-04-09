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

#include "http/http1.h"
#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_request.h"

#include "http/http_option.h"

using namespace std;       // NOLINT
using namespace net::http; // NOLINT

int main() {
  // net::http1::server server{"127.0.0.1", 1280};
  // net::http1::start_server(server);

  net::http::http_option option;
  net::http::http_metric metric;
  net::http::http1_client_request req;
  net::http::http1_server_request req1;
  net::http::http1::message_parser<http1_client_request> parser{&req};
  net::http::http1::message_parser<http1_server_request> parser2{&req1};


  fmt::println("hello world");
  fmt::println("email: xiaomingZhang2020@outlook.com");

  return 0;
}
