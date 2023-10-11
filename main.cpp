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
#include <any>
#include <fmt/format.h>
#include <iostream>

/* #include <exec/variant_sender.hpp> */

#include <span>
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
#include "tcp/tcp_connection.h"
#include <utils/stdexec_util.h>

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

struct any_receiver {
  template <class Sender>
  auto set_next(exec::set_next_t, Sender&&) noexcept {
    return stdexec::just();
  }

  void set_value(stdexec::set_value_t, auto&&...) && noexcept {
  }

  void set_stopped(stdexec::set_stopped_t) && noexcept {
  }

  template <class E>
  void set_error(stdexec::set_error_t, E&&) && noexcept {
  }

  stdexec::empty_env get_env(stdexec::get_env_t) const noexcept {
    return {};
  }
};

ex::sender auto WaitToAlarm(io_uring_context& ctx, uint32_t milliseconds) noexcept {
  return exec::schedule_after(ctx.get_scheduler(), std::chrono::milliseconds(milliseconds));
}

int main() {

  static_assert(__nothrow_decay_copyable<any_receiver>);

  io_uring_context ctx;


  using TcpSocketHandle = sio::io_uring::socket_handle<sio::ip::tcp>;
  TcpSocketHandle socket{ctx, 1, sio::ip::tcp::v4()};

  std::string buffer;
  buffer.resize(1024);
  auto s = std::span(buffer);
  auto ss = std::as_writable_bytes(s);

  auto read = sio::async::read_some(socket, ss);
  // auto read = just(1000);

  auto alarm = WaitToAlarm(ctx, 5000) //
             | let_value([] { return ex::just_error(false); });

  std::jthread thr([&ctx] { ctx.run_until_empty(); });

  auto x = ex::when_any(read, alarm) //
         | then([](std::size_t nbytes) {
             //
             ::printf("nbytes: %d\n", nbytes); // NOLINT
           })                                  //
         | upon_error([]<class E>(E f) {
             if constexpr (std::same_as<E, bool>) {
               cout << f << endl;
             }
           });

  sync_wait(std::move(x));


  // exec::io_uring_context ctx;
  // auto sche = ctx.get_scheduler();
  //
  // std::jthread j([&]() { ctx.run_until_empty(); });
  //
  //
  // sender auto s0 = when_any(just(true));
  // sender auto s1 = then(std::move(s0), [](bool) { return 1; });
  // sender auto s = repeat_effect_until(std::move(s1));
  // // sender auto s1 = then(when_any(just(true)), [] {});
  // stdexec::sync_wait(std::move(s));
  //
  // sender auto x1 = repeat_effect_until(when_any(just(true)));
  // stdexec::connect(std::move(x1), any_receiver{});

  // sync_wait(std::move(x1));

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
