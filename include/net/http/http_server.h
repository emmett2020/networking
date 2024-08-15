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
#include <magic_enum.hpp>

#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <sio/ip/udp.hpp>

#include "net/http/http_common.h"
#include "net/http/http_error.h"
#include "net/http/http_metric.h"
#include "net/http/http_option.h"
#include "net/http/http_request.h"
#include "net/http/http_response.h"
#include "net/http/http_handler.h"
#include "net/utils/flat_buffer.h"

namespace net::http {
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;
  using tcp_acceptor_t = sio::io_uring::acceptor<sio::ip::tcp>;
  using tcp_acceptor_handle_t = sio::io_uring::acceptor_handle<sio::ip::tcp>;
  using udp_socket = sio::io_uring::socket_handle<sio::ip::udp>;
  using udp_acceptor_t = sio::io_uring::acceptor<sio::ip::udp>;
  using udp_acceptor_handle_t = sio::io_uring::acceptor_handle<sio::ip::udp>;

  struct http_connection;

  // A http server.
  struct server {
    using execution_context = exec::io_uring_context;
    using http_handlers = std::vector<std::vector<handler_pattern>>;

    server(execution_context& ctx, const sio::ip::address& addr, port_t port) noexcept
      : server(ctx, sio::ip::endpoint{addr, port}) {
    }

    server(execution_context& ctx, const sio::ip::endpoint& endpoint) noexcept
      : endpoint{endpoint}
      , context{ctx}
      , acceptor{&ctx, endpoint.is_v4() ? sio::ip::tcp::v4() : sio::ip::tcp::v6(), endpoint} {
      constexpr int total_methods_count = magic_enum::enum_count<http::http_method>();
      handlers.resize(total_methods_count);
    }

    void register_handler(http_method method, const std::string& url, const http_handler& handler) {
      auto method_idx = magic_enum::enum_index(method);
      if (!method_idx) {
        throw http_error("unsupport http_method");
      }
      handlers[*method_idx].emplace_back(url, handler);
    }

    void register_handler(unsigned methods, const std::string& url, const http_handler& handler) {
      int method_idx = 0;
      while (methods > 0) {
        auto enabled = static_cast<bool>(methods & 0x1);
        methods = methods >> 1;
        ++method_idx;
        if (enabled) {
          handlers[method_idx].emplace_back(url, handler);
        }
      }
    }

    sio::ip::endpoint endpoint;
    execution_context& context; // NOLINT
    tcp_acceptor_t acceptor;
    server_metric metric;
    http_handlers handlers;
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
    utils::flat_buffer<65535> buffer;
  };


} // namespace net::http
