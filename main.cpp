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
#include <fmt/format.h>
#include <iostream>

/* #include <exec/variant_sender.hpp> */

#include <stdexec/execution.hpp>
#include "exec/linux/io_uring_context.hpp"
#include "sio/io_uring/socket_handle.hpp"

#include "sio/ip/endpoint.hpp"
#include "sio/ip/tcp.hpp"
#include "sio/net_concepts.hpp"
#include "sio/sequence/ignore_all.hpp"
#include "sio/sequence/let_value_each.hpp"

using namespace std;  // NOLINT

struct Foo {
  std::string name;

  ~Foo() { std::cout << "~Foo()" << std::endl; }
};

using stdexec::just;
using stdexec::let_value;
using stdexec::sync_wait;
using stdexec::then;

auto Func2() {
  stdexec::sync_wait(just(3)                                //
                     | then([](int n) { return n; })        //
                     | then([](int n) { return n; })        //
                     | then([](int n) { return just(n); })  //
                     | then([](auto n) { sync_wait(n); }));

  stdexec::sync_wait(just(3)  //
                     | let_value([](int n) {
                         return just(1)                          //
                                | then([](int m) { return m; })  //
                                | then([n](int m) {              //
                                    return m + n;
                                  });
                       })                //
                     | then([](int n) {  //
                       }));

  // std::cout << q << std::endl;
}

auto Fun() {
  return just(Foo{.name = "Foo"}) | then([](Foo foo) {
           foo.name = "new name";
           return 1;
           // return just(1);
         })                                          //
         | let_value([](int n) { return just(n); })  //
         | then([](int n) {
             cout << n << endl;
             return n;
           })  //
         | let_value([](int n) { return just(n); });
}

struct F {
  std::string f;
};

// B could only be construct by F. And B doesn't have default constructor.
struct B {
  explicit B(F& f) {}
};

int main() {
  // Construct B needs F already be constructed, but in this case, F is
  // construct with B at the same time.
  // Thus follow line couldn't work:
  // stdexec::just(1, string{"2"}, F{}, B{});
  // The follow line should work, but does we have a better way?

  stdexec::sender auto s =                //
      stdexec::just(1, string{"2"}, F{})  //
      | stdexec::let_value([](int i, const string& m, F f) {
          stdexec::just(B{f})  //
              | stdexec::let_value([i, &m, &f](B& b) {
                  // We can safely use i, m, f, b.
                });
        });

  // stdexec::sender auto s = prepare_server(server)                   //
  //                          | stdexec::then(load_config())           //
  //                          | stdexec::then(register_handler())      //
  //                          | stdexec::then(some_op())               //
  //                          | stdexec::then(start_listening())       //
  //                          | stdexec::unop_error(error_handler());  //

  // stdexec::sync_wait(std::move(s));
  std::cout << "hello world" << std::endl;
  std::cout << "you can find me by xiaomingZhang2020@outlook.com" << std::endl;
  std::cout << std::endl;

  return 0;
}
