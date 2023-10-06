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

/* #include <exec/variant_sender.hpp> */

#include <stdexec/execution.hpp>
#include <thread>
#include "exec/linux/io_uring_context.hpp"
#include "exec/repeat_effect_until.hpp"
#include "exec/timed_scheduler.hpp"
#include "exec/when_any.hpp"
#include "sio/io_uring/socket_handle.hpp"

#include "sio/ip/endpoint.hpp"
#include "sio/ip/tcp.hpp"
#include "sio/net_concepts.hpp"
#include "sio/sequence/ignore_all.hpp"
#include "sio/sequence/let_value_each.hpp"

using namespace std; // NOLINT

using stdexec::just;
using stdexec::let_value;
using stdexec::sync_wait;
using stdexec::then;
using exec::when_any;
using stdexec::when_all;
using namespace chrono_literals;
using namespace stdexec; // NOLINT
using namespace exec;    // NOLINT
using namespace std;     // NOLINT

int main() {

  exec::io_uring_context ctx;
  auto sche = ctx.get_scheduler();

  std::jthread j([&]() { ctx.run_until_empty(); });

  // sender auto s = when_any(repeat_effect_until(just(false)));
  // sync_wait(std::move(s));
  //
  stdexec::sync_wait(exec::when_any(
    exec::schedule_after(sche, 1s) | stdexec::then([] { std::cout << "Hello, 1!\n"; })
      | stdexec::upon_stopped([] { std::cout << "Hello, 1, stopped.\n"; }),
    exec::schedule_after(sche, 500ms) | stdexec::then([] { std::cout << "Hello, 2!\n"; })
      | stdexec::upon_stopped([] { std::cout << "Hello, 2, stopped.\n"; })));


  // sender auto s33 = schedule_after(sche, 1s) | then([] {});
  // sender auto s4 = when_any(s33, s33);
  // sender auto s5 = stdexec::then(std::move(s4), [](auto&&...) { return true; });
  // sender auto s6 = exec::repeat_effect_until(std::move(s5));
  // sync_wait(std::move(s6));

  // sender auto t1 = when_any(schedule_after(sche, 1s) | then([] noexcept { return true; }));
  // sender auto t1 = when_any(schedule_after(sche, 1s));
  // sender auto t2 = stdexec::then(std::move(t1), [](auto&&...) noexcept { return true; });
  // sender auto t3 = exec::repeat_effect_until(std::move(t2));
  // sender auto t4 = then(std::move(t3), [](auto&&...) noexcept {});
  // sync_wait(std::move(t4));
  //
  //
  // auto p1 = when_any(schedule_after(sche, 1s) | then([] noexcept {})) //
  //         | then([](auto&&...) { return true; })                      //
  //         | repeat_effect_until()                                     //
  //         | then([](auto&&...) { return 1; });
  // sync_wait(std::move(p1));


  // -----------------------------------------------------------

  // sender auto s = exec::when_any(exec::schedule_after(sche, 5s) | then([] {
  //                                  cout << "5s";
  //                                  return 1;
  //                                })); //

  // sender auto s2 = exec::when_any(just(true));
  // sender auto s3 = repeat_effect_until(std::move(s2));

  // sync_wait(std::move(s3));

  // sender auto s1 = then(std::move(s), [](int n) { cout << "when_any ok\n"; });
  // sync_wait(when_any(std::move(s), ctx.run(exec::until::empty)));

  // exec::repeat_effect_until(std::move(s));


  // stdexec::sender auto s = prepare_server(server)                   //
  //                          | stdexec::then(load_config())           //
  //                          | stdexec::then(register_handler())      //
  //                          | stdexec::then(some_op())               //
  //                          | stdexec::then(start_listening())       //
  //                          | stdexec::unop_error(error_handler());  //

  // stdexec::sync_wait(std::move(s));
  std::cout << "hello world" << std::endl;
  std::cout << "you can find me by xiaomingZhang2020@outlook.com" << std::endl;
  std::cout << std::endl;

  return 0;
}

// sender auto s1 = exec::when_any(exec::schedule_after(sche, 1s), exec::schedule_after(sche, 1ms))
//                | stdexec::then([](auto&&...) { return std::error_code(); })
//                | then([](auto&&...) { return true; }) //
//                | exec::repeat_effect_until() | let_value([](auto&&...) { return just(); });
// sync_wait(std::move(s1));
//
//
