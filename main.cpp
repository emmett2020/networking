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

int main() {
  constexpr std::string_view ip = "127.0.0.1";
  constexpr net::http::port_t port = 8080;
  fmt::println("start listening on {}:{}", ip, port);
  net::http::server server{ip, port};
  net::http::start_server(server);
  return 0;
}
