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

#include <string>

namespace net::http {
  //

  template <class HashTable>
  class basic_hashtable {
    constexpr virtual bool add(const std::string& value) = 0;
    constexpr virtual bool del(const std::string& value) = 0;
    constexpr virtual bool update(const std::string& value) = 0;
    constexpr virtual bool get_value(const std::string& value) = 0;



   private:
    HashTable hash_table_;
  };

} // namespace net::http
