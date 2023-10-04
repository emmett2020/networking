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

#include <array>
#include <cstddef>
#include <span>

template <class T, std::size_t Capacity>
class CircularArray {

  template <std::size_t CommitSize>
  std::size_t Commit(std::span<T, CommitSize> s);

  bool Consume(std::size_t consume_size) noexcept {
    if (tail_ + consume_size > head_) {
      return false;
    }
    tail_ += consume_size;
    return true;
  }

  [[nodiscard]] std::size_t Size() const noexcept { return head_ - tail_; }

 private:
  std::size_t head_{0};
  std::size_t tail_{0};
  std::array<T, Capacity> array_{0};
};
