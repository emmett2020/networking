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

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cstdint>
#include <exception>
#include <utility>


#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <sio/io_concepts.hpp>

#include "http/http_metric.h"
#include "http/http_option.h"
#include "utils/execution.h"
#include "http/v1/http1_request.h"
#include "utils/flat_buffer.h"
#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_response.h"
#include "utils/if_then_else.h"
#include "http/http_common.h"
#include "http/http_error.h"
#include "http/v1/http1_op_recv.h"
#include "http/v1/http1_op_send.h"
#include "http/v1/http1_op_handle.h"

// TODO: APIs should be constraint by sender_of concept
// TODO: refine headers

namespace net::http::http1 {

  using namespace std::chrono_literals;
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;

  // A http session is a conversation between client and server.
  // We use session id to identify a specific unique conversation.
  struct http_session {
    // TODO: Just a simple session id factory which must be replaced in future.
    using id_type = uint64_t;

    static id_type make_session_id() noexcept {
      static std::atomic_int64_t session_idx{0};
      ++session_idx;
      return session_idx.load();
    }

    tcp_socket socket;
    id_type id{make_session_id()};
    std::size_t reuse_cnt = 0;
    http_option option;
    http_metric recv_metric;
    http_metric send_metric;
  };

  // A http server.
  struct server {
    using context_t = ex::io_uring_context;
    using acceptor_handle_t = sio::io_uring::acceptor_handle<sio::ip::tcp>;
    using acceptor_t = sio::io_uring::acceptor<sio::ip::tcp>;
    using socket_t = sio::io_uring::socket_handle<sio::ip::tcp>;

    server(context_t& ctx, std::string_view addr, port_t port) noexcept
      : server(ctx, sio::ip::endpoint{sio::ip::make_address_v4(addr), port}) {
    }

    server(context_t& ctx, sio::ip::address addr, port_t port) noexcept
      : server(ctx, sio::ip::endpoint{addr, port}) {
    }

    explicit server(context_t& ctx, sio::ip::endpoint endpoint) noexcept
      : endpoint{endpoint}
      , context(ctx)
      , acceptor{&context, sio::ip::tcp::v4(), endpoint} {
    }

    sio::ip::endpoint endpoint;
    context_t& context; // NOLINT
    acceptor_t acceptor;
    server_metric metric;
  };

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

  inline ex::sender auto deal_one(server& serv, http_session& session) {
    auto update_recv_metric = [&](http1_client_request&& req) {
      serv.metric.total_recv_size += req.metric.size.total;
      session.recv_metric.size.total += req.metric.size.total;
      return std::move(req);
    };

    auto update_send_metric = [&](http1_client_response&& rsp) {
      serv.metric.total_recv_size += rsp.metric.size.total;
      session.recv_metric.size.total += rsp.metric.size.total;
      return std::move(rsp);
    };

    return recv_request(session.socket, session.option) //
         | ex::then(update_recv_metric)                 //
         | ex::let_value(handle_request)                //
         | ex::let_value([&](http1_client_response& resp) {
             return send_response(session.socket, session.option, std::move(resp));
           })                           //
         | ex::then(update_send_metric) //
         | ex::then([&](http1_client_response&& rsp) {
             session.option.need_keepalive = rsp.need_keepalive;
             return !rsp.need_keepalive;
           })
         | ex::repeat_effect_until() //
         | ex::upon_error(handle_error);
  }

  inline void start_server(server& s) noexcept {
    auto handles = sio::async::use_resources(
      [&](server::acceptor_handle_t acceptor) noexcept {
        return sio::async::accept(acceptor) //
             | sio::let_value_each([&](server::socket_t socket) {
                 return ex::just(http_session{.socket = socket}) //
                      | ex::let_value([&](auto& session) { return deal_one(s, session); });
               })
             | sio::ignore_all();
      },
      s.acceptor);
    ex::sync_wait(exec::when_any(handles, s.context.run()));
  }

} // namespace net::http::http1

namespace net::http {
  using server = net::http::http1::server;
  using http1::start_server;
}
