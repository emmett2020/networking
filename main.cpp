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
  // exec::io_uring_context ctx;
  // auto sche = ctx.get_scheduler();
  // std::jthread j([&]() { ctx.run_until_empty(); });
  net::tcp::Server server{"127.0.0.1", 1280};
  server.Run();


  // stdexec::sync_wait(std::move(s));
  std::cout << "hello world\n";
  std::cout << "you can find me by xiaomingZhang2020@outlook.com\n";

  return 0;
}
