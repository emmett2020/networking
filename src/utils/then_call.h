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
#include <utility>

// then_call


template <typename Func>
concept is_member_function = std::is_member_function_pointer_v<Func>;
//
template <typename T>
concept class_type = std::is_class_v<T>;

// NOLINTNEXTLINE
struct then_call_t {
  template <stdexec::sender Prev, stdexec::__movable_value Func, typename... Args>
  auto operator()(Prev&& prev, Func func, Args&&... arguments) const {
    auto wrapper =
      [func,
       args_tuple = std::tuple<Args...>(std::forward<Args>(arguments)...)]<typename... PrevRes>(
        PrevRes&&... result) mutable noexcept {
        return std::apply(
          [func, &result...](auto&... args) {
            return func(static_cast<PrevRes&&>(result)..., std::forward<Args>(args)...);
          },
          args_tuple);
      };

    return stdexec::then(static_cast<Prev&&>(prev), wrapper);
  }

  template <stdexec::__movable_value Func, typename... Args>
  stdexec::__binder_back<then_call_t, Func, Args&&...> operator()(Func func, Args&&... args) const {
    return {
      {},
      {},
      {static_cast<Func&&>(func), static_cast<Args&&>(args)...}
    };
  }

  // call class type
  template < stdexec::sender Prev, class_type ClassType, is_member_function Func, typename... Args> //
  auto operator()(Prev&& prev, ClassType* instance, Func member_func, Args&&... arguments) const {
    auto func =
      [&instance,
       member_func,
       args_tuple = std::tuple<Args...>(std::forward<Args>(arguments)...)]<typename... PrevRes>(
        PrevRes&&... result) mutable noexcept {
        return std::apply(
          [&instance, member_func, &result...](auto&... args) {
            return (instance->*member_func)(
              static_cast<PrevRes&&>(result)..., std::forward<Args>(args)...);
          },
          args_tuple);
      };

    return stdexec::then(static_cast<Prev&&>(prev), func);
  }

  template <class_type ClassType, is_member_function Func, typename... Args>
  stdexec::__binder_back<then_call_t, ClassType*, Func, Args&&...>
    operator()(ClassType* instance, Func member_func, Args&&... args) const {
    return {
      {},
      {},
      {instance, member_func, static_cast<Args&&>(args)...}
    };
  }
};

inline constexpr then_call_t then_call{}; //NOLINT
