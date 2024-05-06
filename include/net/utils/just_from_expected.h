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
#include <exec/variant_sender.hpp>

namespace net::utils {
  // NOTE: std::expected works well with std::execution::just and std::execution::just_error.

  // TODO: add just_from_expected(std::expected exp) overload function.
  // TODO: add just_from_expected(Func&& func1, Func&& func2, ...), this
  //       overloaded function should return (func1_ret, func2_ret, ...) when all functions have value.
  // TODO: add std::expected concept

  template <class Func>
  stdexec::sender auto just_from_expected(Func&& func) noexcept {
    auto ret = std::forward<Func>(func)();
    using ret_t = exec::variant_sender<
      decltype(stdexec::just(ret.value())), //
      decltype(stdexec::just_error(std::error_code(ret.error())))>;
    if (ret.has_value()) {
      return ret_t(stdexec::just(ret.value()));
    }
    return ret_t(stdexec::just_error(std::error_code(ret.error())));
  }

} // namespace net::utils
