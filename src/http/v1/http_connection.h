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

#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>

#include "http/http_metric.h"
#include "http/http_option.h"
#include "http/http_request.h"
#include "http/http_response.h"
#include "utils/flat_buffer.h"
#include "utils/execution.h"

namespace net::http::http1 {
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;

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

  struct http_connection {
    tcp_socket socket;
    std::size_t id = 0;
    std::size_t keepalive_count = 0;
    bool need_keepalive = false;
    http_option option;
    http_metric recv_metric;
    http_metric send_metric;
    http_request request;
    http_response response;
    server* serv = nullptr;
    util::flat_buffer<65535> buffer;
  };


} // namespace net::http::http1
