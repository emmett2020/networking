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
  using tcp_socket_handle = sio::io_uring::socket_handle<sio::ip::tcp>;
  using tcp_acceptor_handle = sio::io_uring::acceptor_handle<sio::ip::tcp>;
  using tcp_acceptor = sio::io_uring::acceptor<sio::ip::tcp>;


  using const_buffer = std::span<const std::byte>;
  using mutable_buffer = std::span< std::byte>;

  static constexpr auto unlimited_timeout = std::chrono::seconds::max();

  // WARN: useless
  inline auto CopyArray(const_buffer buffer) noexcept -> std::string {
    std::string ss;
    for (auto b: buffer) {
      ss.push_back(static_cast<char>(b));
    }
    return ss;
  }

  // for client receive server's response
  struct SocketRecvConfig {
    uint32_t recv_once_max_wait_time_ms{5000};
    uint32_t recv_all_max_wait_time_ms{1000 * 60};
  };

  // for server send response to client
  struct SocketSendConfig {
    uint32_t send_response_line_and_headers_max_wait_time_ms{5000};
    uint32_t send_body_max_wait_time_ms{5000};
    uint32_t send_all_max_wait_time_ms{1000 * 60};
  };

  struct DomainConfig {
    SocketRecvConfig socket_recv_config;
    SocketSendConfig socket_send_config;
  };

  // NOTE: some principle
  // 1. all temporary things use just to hold
  // 2. all long lived things saved in server
  // 3.


  struct SocketSendMeta {
    tcp_socket_handle socket;
    std::string start_line_and_headers;
    http1::Response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
    uint32_t send_once_max_wait_time_ms;
  };

  struct server_metric {
    uint64_t total_size = 0;
  };

  struct recv_metric {
    using duration = std::chrono::seconds;
    using timepoint = std::chrono::time_point<std::chrono::system_clock>;
    timepoint start_timestamp{};
    timepoint end_timestamp{};
    duration io_duration{0};
    uint64_t total_size{0};
    uint32_t recv_cnt{0};
  };

  struct send_metric {
    uint64_t total_size = 0;
  };

  // TODO(xiaoming): could get the information from server yaml config.
  // So it's not constexpr.
  struct recv_option {
    using duration = std::chrono::seconds;
    duration total_timeout{0};
    duration keepalive_timeout{0};
  };

  struct send_option {
    using duration = std::chrono::seconds;
    duration total_timeout{0};
  };

  struct socket_recv_handle {
    using duration = std::chrono::seconds;
    using timepoint = std::chrono::time_point<std::chrono::system_clock>;

    tcp_socket_handle socket;
    std::array<std::byte, 8192> recv_buffer{};
    std::size_t unparsed_size{0};
    http1::Request request{};
    http1::RequestParser parser{&request};
    std::error_code ec{};
    std::uint64_t recv_cnt = 0;
    recv_metric metric{};
    recv_option opt{};
    duration remaining_timeout;
    timepoint start_recv_timepoint;
  };

  // tcp session.
  struct tcp_session {
    explicit tcp_session(tcp_socket_handle socket)
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
    tcp_socket_handle socket;
    http1::Request request{};
    http1::Response response{};
    std::size_t reuse_cnt = 0;
  };

  // TODO(xiaoming): how to make ipv6?
  struct Server {
    Server(std::string_view addr, uint16_t port) noexcept
      : Server(sio::ip::endpoint{sio::ip::make_address_v4(addr), port}) {
    }

    Server(sio::ip::address addr, uint16_t port) noexcept
      : Server(sio::ip::endpoint{addr, port}) {
    }

    explicit Server(sio::ip::endpoint endpoint) noexcept
      : endpoint{endpoint}
      , acceptor{&context, sio::ip::tcp::v4(), endpoint} {
    }

    void update_metric(recv_metric metric) {
    }

    void update_metric(send_metric metric) {
    }

    exec::io_uring_context context{};
    sio::ip::endpoint endpoint{};
    tcp_acceptor acceptor;
    server_metric metric;
    recv_option recv_opt;
    send_option send_opt;
  };

  void start_server(Server& server) noexcept;

} // namespace net::tcp
