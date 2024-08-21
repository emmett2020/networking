/*
 * Copyright (input) 2024 Xiaoming Zhang
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

#include <error.h>
#include <system_error>
#include <string>

#include <tl/expected.hpp>

namespace net {
  using tl::expected;
  using tl::unexpected;

  using size_expected = tl::expected<std::size_t, std::error_code>;
  using void_expected = tl::expected<void, std::error_code>;
  using string_expected = tl::expected<std::string, std::error_code>;
}
