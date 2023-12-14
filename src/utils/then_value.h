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

#include <stdexec/execution.hpp>
#include <stdexec/concepts.hpp>
#include <type_traits>

template <typename T, typename Func>
concept member_function_of = std::is_member_function_pointer<Func T::*>::value;

// NOLINTNEXTLINE
struct then_value_t {
  template <stdexec::sender Prev, stdexec::__movable_value Func, typename... Args>
  auto operator()(Prev&& prev, Func func, Args&&... args) const {
    return stdexec::let_value(
      static_cast<Prev&&>(prev), [func, &args...](auto&&... result) noexcept { //
        using RetType = decltype(func(std::move(result)..., args...));
        if constexpr (std::same_as<RetType, void>) {
          func(std::move(result)..., args...);
          return stdexec::just();
        } else {
          return stdexec::just(func(std::move(result)..., args...));
        }
      });
  }

  template <stdexec::__movable_value Func, typename... Args>
  stdexec::__binder_back<then_value_t, Func, Args&&...>
    operator()(Func func, Args&&... args) const {
    return {
      {},
      {},
      {static_cast<Func&&>(func), static_cast<Args&&>(args)...}
    };
  }

  // call class type
  template <stdexec::sender Prev, typename ClassType, typename Func, typename... Args>
    requires(std::is_class_v<ClassType>)
  auto operator()(Prev&& prev, ClassType& instance, Func member_func, Args&&... args) const {
    auto func = [&instance, member_func]<typename... MemberArgs>(MemberArgs&&... member_args) {
      (instance.*member_func)(static_cast<MemberArgs&&>(member_args)...);
    };

    return stdexec::let_value(
      static_cast<Prev&&>(prev), [func, &args...](auto&&... result) noexcept { //
        using RetType = decltype(func(std::move(result)..., static_cast<Args&&>(args)...));
        if constexpr (std::same_as<RetType, void>) {
          func(std::move(result)..., args...);
          return stdexec::just();
        } else {
          return stdexec::just(func(std::move(result)..., args...));
        }
      });
  }
};

inline constexpr then_value_t then_value{}; //NOLINT
