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
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

#include <exec/linux/io_uring_context.hpp>
#include <exec/repeat_effect_until.hpp>
#include <exec/when_any.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/endpoint.hpp>
#include <sio/ip/tcp.hpp>
#include <sio/sequence/ignore_all.hpp>
#include <sio/sequence/let_value_each.hpp>
#include <sio/io_concepts.hpp>
#include <sio/ip/address.hpp>
#include <stdexec/execution.hpp>
#include <utility>

#include "exec/timed_scheduler.hpp"
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "http1/http_response.h"
#include "utils/if_then_else.h"
#include "utils/stdexec_util.h"

namespace net::tcp {
  using namespace std::chrono_literals;
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;
  using tcp_acceptor_handle = sio::io_uring::acceptor_handle<sio::ip::tcp>;
  using tcp_acceptor = sio::io_uring::acceptor<sio::ip::tcp>;

  using const_buffer = std::span<const std::byte>;
  using mutable_buffer = std::span< std::byte>;

  static constexpr auto unlimited_timeout = std::chrono::seconds::max();

  struct send_state {
    tcp_socket socket;
    std::string start_line_and_headers;
    http1::Response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
    uint32_t send_once_max_wait_time_ms;
  };

  // tcp session.
  struct session {
    explicit session(tcp_socket socket)
      : socket(socket)
      , id(make_session_id())
      , context(socket.context_) {
    }

    using session_id = int64_t;

    static session_id make_session_id() noexcept {
      static std::atomic_int64_t session_idx{0};
      ++session_idx;
      return session_idx.load();
    }

    exec::io_uring_context* context{nullptr};
    session_id id = 0;
    tcp_socket socket;
    http1::client_request request{};
    http1::Response response{};
    std::size_t reuse_cnt = 0;
  };

  struct recv_state {
    http1::client_request request{};
    http1::RequestParser parser{&request};
    std::array<std::byte, 8192> buffer{};
    uint32_t unparsed_size{0};
    std::chrono::seconds remaining_time;
  };

  // TODO(xiaoming): how to make ipv6?
  struct server {
    server(std::string_view addr, uint16_t port) noexcept
      : server(sio::ip::endpoint{sio::ip::make_address_v4(addr), port}) {
    }

    server(sio::ip::address addr, uint16_t port) noexcept
      : server(sio::ip::endpoint{addr, port}) {
    }

    explicit server(sio::ip::endpoint endpoint) noexcept
      : endpoint{endpoint}
      , acceptor{&context, sio::ip::tcp::v4(), endpoint} {
    }

    exec::io_uring_context context{};
    sio::ip::endpoint endpoint{};
    tcp_acceptor acceptor;
  };

  void start_server(server& server) noexcept;

} // namespace net::tcp
