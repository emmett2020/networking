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
#include <iostream>
#include <stdexec/execution.hpp>
using namespace std;  // NOLINT

// namespace net {}

int main() {
  std::cout << "hello world" << std::endl;
  std::cout << "you can find me by xiaomingZhang2020@outlook.com" << std::endl;
  fmt::print("hello world1\n");
  std::cout << std::endl;

  // net::epoll_context io_context;
  // net::http::server server{};

  // stdexec::sender auto s = prepare_server(server)                   //
  //                          | stdexec::then(load_config())           //
  //                          | stdexec::then(register_handler())      //
  //                          | stdexec::then(some_op())               //
  //                          | stdexec::then(start_listening())       //
  //                          | stdexec::unop_error(error_handler());  //

  // stdexec::sync_wait(std::move(s));
  return 0;
}
