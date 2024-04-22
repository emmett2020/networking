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

#include <chrono>
#include <cstdint>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>

#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>
#include <exec/linux/io_uring_context.hpp>
#include <exec/timed_scheduler.hpp>
#include <exec/repeat_effect_until.hpp>
#include <sio/io_concepts.hpp>
#include <stdexec/execution.hpp>

#include "http/v1/http1_request.h"
#include "utils/flat_buffer.h"
#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_response.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

// TODO: APIs should be constraint by sender_of concept

namespace net::http::http1 {

  using namespace std::chrono_literals;
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;
  using parser_t = message_parser<http1_client_request>;

  // Parse HTTP request uses receive buffer.
  ex::sender auto parse_request(parser_t& parser, util::flat_buffer<8192>& buffer) noexcept;

  ex::sender auto recv_request(const tcp_socket& socket) noexcept;

  ex::sender auto handle_request(const http1_client_request& request) noexcept;

  ex::sender auto send_response(const tcp_socket& socket, http1_client_response&& resp) noexcept;

  // A http session is a conversation between client and server.
  // We use session id to identify a specific unique conversation.
  struct session {
    // TODO: Just a simple session id factory which must be replaced in future.
    using id_type = uint64_t;

    static id_type make_session_id() noexcept {
      static std::atomic_int64_t session_idx{0};
      ++session_idx;
      return session_idx.load();
    }

    id_type id{make_session_id()};
    tcp_socket socket;
    http1_client_request request{};
    http1_client_response response{};
    std::size_t reuse_cnt{0};
  };

  // A http server.
  struct server {
    using context_type = ex::io_uring_context;
    using request_type = http1_client_request;
    using response_type = http1_client_response;
    using acceptor_handle_type = sio::io_uring::acceptor_handle<sio::ip::tcp>;
    using acceptor_type = sio::io_uring::acceptor<sio::ip::tcp>;
    using socket_type = sio::io_uring::socket_handle<sio::ip::tcp>;
    using session_type = session;

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

    sio::ip::endpoint endpoint{};
    context_type context{};
    acceptor_type acceptor;
  };

  void start_server(server& s) noexcept;
} // namespace net::http::http1

namespace net::http {
  using server = net::http::http1::server;
  using http1::start_server;
}
