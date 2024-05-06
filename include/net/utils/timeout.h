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

#pragma once

#include <chrono>
#include <stdexec/execution.hpp>
#include <stdexec/concepts.hpp>
#include <exec/when_any.hpp>
#include <exec/inline_scheduler.hpp>
#include <exec/timed_scheduler.hpp>
#include <utility>

// TODO: need a robust implementation according to P2300 revision 8.

namespace net::utils {
  struct timeout_t {
    // If sender supports cancellation, no need to pass scheduler to function
    template <stdexec::sender Sender, stdexec::scheduler Scheduler>
    stdexec::sender auto
      operator()(Sender &&sndr, Scheduler scheduler, exec::duration_of_t<Scheduler> timeout) const {
      auto start = std::chrono::system_clock::now();
      auto alarm = exec::schedule_after(scheduler, timeout) //
                 | stdexec::let_value([] { return stdexec::just_stopped(); });
      auto task = stdexec::let_value(std::move(sndr), [&start](auto &&...values) {
        return stdexec::just(start, std::chrono::system_clock::now(), std::move(values)...);
      });

      return exec::when_any(std::move(alarm), std::move(task));
    }

    template <class Schduler, class Dur>
    stdexec::__binder_back<timeout_t, Schduler, Dur>
      operator()(Schduler scheduler, Dur timeout) const noexcept {
      return {
        {},
        {},
        {scheduler, timeout}
      };
    }
  };

  inline constexpr timeout_t timeout{}; // NOLINT

  // int main() {
  //   io_uring_context ctx;
  //   std::jthread thr{[&] {
  //     ctx.run_until_stopped();
  //   }};
  //
  //   {
  //     scope_guard guard{[&]() noexcept {
  //       ctx.request_stop();
  //     }};
  //     auto sche = ctx.get_scheduler();
  //
  //     auto sndr = just()         //
  //               | let_value([] { //
  //                   std::getchar();
  //                   return just(1);
  //                 })                                          //
  //               | timeout(sche, 1000ms)                       //
  //               | let_value([](auto start, auto end, int n) { //
  //                   std::cout << (end - start).count() << std::endl;
  //                   std::cout << "n=" << n << std::endl;
  //                   return just();
  //                 }) //
  //               | upon_stopped([] { std::cout << "stopped\n"; });
  //
  //     sync_wait(std::move(sndr));
  //   }
  //
  //   return 1;
  // }

} // namespace net::utils
