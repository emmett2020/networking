/* Copyright 2023 Xiaoming Zhang
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

#include <catch2/catch_test_macros.hpp>
#include <map>

#include "utils/string_compare.h"

using namespace net::util;

TEST_CASE("Test strcasecomp could work.", "[strcasecomp]") {
  CHECK(net::util::strcasecmp("Host", "Host"));
}

TEST_CASE("Test case insensitive map could work.", "[case_insensitive_map]") {
  std::map<std::string, std::string, net::util::case_insensitive_compare> m;
  m["key"] = "val";
  CHECK(m.contains("Key"));
}

TEST_CASE("Test case insensitive multimap could work.", "[case_insensitive_multimap]") {
  std::multimap<std::string, std::string, net::util::case_insensitive_compare> m;
  m.emplace("key", "val");
  CHECK(m.contains("Key"));
}
