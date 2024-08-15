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

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

#include <exception>
#include <utility>


#include <exec/linux/io_uring_context.hpp>
#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <sio/io_concepts.hpp>
#include <sio/net_concepts.hpp>

#include "net/http/http_metric.h"
#include "net/http/http_option.h"
#include "net/http/http_request.h"
#include "net/http/http_server.h"
#include "net/http/v1/http1_op_recv.h"
#include "net/http/v1/http1_op_send.h"
#include "net/http/v1/http1_op_handle.h"

// TODO: APIs should be constraint by sender_of concept
// TODO: refine headers

namespace net::http::http1 {

  using namespace std::chrono_literals;

  struct handle_error_t {
    template <typename E>
    auto operator()(E e) noexcept {
      if constexpr (std::same_as<E, std::error_code>) {
        fmt::println("Error: {}", e.message());
      } else if constexpr (std::same_as<E, std::exception_ptr>) {
        try {
          std::rethrow_exception(e);
        } catch (const std::exception& ptr) {
          fmt::println("Error: {}", ptr.what());
        }
      } else {
        fmt::println("Unknown error!");
      }
    }
  };

  inline constexpr handle_error_t handle_error{};

  inline http_connection&& update_recv_metric(http_connection&& conn) {
    const http_request& req = conn.request;
    conn.serv->metric.total_recv_size += req.metric.size.total;
    conn.recv_metric.size.total += req.metric.size.total;
    return std::move(conn);
  }

  inline http_connection&& update_send_metric(http_connection&& conn) {
    return std::move(conn);
  }

  inline bool check_keepalive(http_connection&& conn) {
    conn.option.need_keepalive = conn.need_keepalive;
    return !conn.need_keepalive;
  }

  inline ex::sender auto deal_one(http_connection&& conn) {
    return recv_request(std::move(conn)) //
         | ex::then(update_recv_metric)  //
         | ex::let_value(handle_request) //
         | ex::let_value(valid_response) //
         | ex::let_value(send_response)  //
         | ex::then(update_send_metric)  //
         | ex::then(check_keepalive)     //
         | exec::repeat_effect_until()   //
         | ex::upon_error(handle_error);
  }

  inline void start_server(server& s) noexcept {
    auto handles = sio::async::use_resources(
      [&](server::acceptor_handle_t acceptor) noexcept {
        return sio::async::accept(acceptor) //
             | sio::let_value_each([&](server::socket_t socket) {
                 return ex::just(http_connection{.socket = socket, .serv = &s}) //
                      | ex::let_value([&](auto& conn) { return deal_one(std::move(conn)); });
               })
             | sio::ignore_all();
      },
      s.acceptor);
    ex::sync_wait(exec::when_any(handles, s.context.run()));
  }

} // namespace net::http::http1

namespace net::http {
  using http1::start_server;
}
