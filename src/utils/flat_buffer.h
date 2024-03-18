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

#include <array>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>

namespace net::util {

  /*
 * flat_buffer:
 * --------------------------------------------------
 * |          | readable region | writable region   |
 * --------------------------------------------------
 * 0          read_             write_             capacity
 *
*/

  // Capacity: The maximum size of a buffer. it's also the initialize size in static flat buffer.
  // Required: The minimum size of writable region. If the left size doesn't meet this requirment,
  //           buffer will be automatically rearranged. If the requirement is still not met, std::length_error
  //           will be thrown.
  template <std::size_t Capacity, std::size_t Required = 512>
  class flat_buffer {
   public:
    // Return the underlying data size.
    [[nodiscard]] std::size_t capacity() const noexcept {
      return Capacity;
    }

    // Return the number of required minimum writable region size.
    [[nodiscard]] std::size_t required_size() const noexcept {
      return Required;
    }

    // Return the number of readable bytes.
    [[nodiscard]] std::size_t readable_size() const noexcept {
      return write_ - read_;
    }

    // Return the number of writable bytes.
    [[nodiscard]] std::size_t writable_size() const noexcept {
      return Capacity - write_;
    }

    // Return a constant buffer sequence representing the readable bytes.
    [[nodiscard]] std::span<std::byte> rbuffer() const noexcept {
      return std::span(data_).subspan(read_, readable_size());
    }

    // Check requirements and rearrange buffer to get enough contiguous linear writable region if requirements check failed.
    void prepare() {
      if (writable_size() >= Required) [[likely]] {
        // Existing capacity is sufficient.
        return;
      }

      const auto rsize = readable_size();
      if (capacity() - rsize < Required) [[unlikely]] {
        throw std::length_error{"buffer overflow"};
      }
      if (rsize > 0) [[likely]] {
        std::memmove(0, read_, rsize);
      }
      read_ = 0;
      write_ = read_ + rsize;
    }

    // Return a mutable buffer sequence representing writable bytes.
    std::span<std::byte> wbuffer() noexcept {
      return std::span(data_).subspan(write_, writable_size());
    }

    // Append writable bytes to the readable bytes.
    void commit(std::size_t n) noexcept {
      write_ += std::min<std::size_t>(n, Capacity - write_);
    }

    // Remove bytes from beginning of the readable bytes.
    void consume(std::size_t n) noexcept {
      // If n more than readable bytes, then all readable bytes will be consumed.
      if (n >= readable_size()) {
        read_ = 0;
        write_ = 0;
        return;
      }
      read_ += n;
    }


   private:
    std::size_t read_{0};
    std::size_t write_{0};
    std::array<std::byte, Capacity> data_{};
  };

} // namespace net::util
