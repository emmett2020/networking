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
  using TcpSocketHandle = sio::io_uring::socket_handle<sio::ip::tcp>;
  using TcpAcceptorHandle = sio::io_uring::acceptor_handle<sio::ip::tcp>;
  using TcpAcceptor = sio::io_uring::acceptor<sio::ip::tcp>;

  using SessionId = std::string;

  struct Session {
    explicit Session(std::string session_id, TcpSocketHandle socket_handle)
      : session_id_(std::move(session_id))
      , socket_handle_(socket_handle) {
    }

    Session(Session&& other) noexcept
      : session_id_(std::move(other.session_id_))
      , socket_handle_(other.socket_handle_)
      , request_(std::move(other.request_))
      , response_(std::move(other.response_)) {
    }

    Session& operator=(Session&& other) noexcept {
      this->session_id_ = std::move(other.session_id_);
      this->socket_handle_ = other.socket_handle_;
      this->request_ = std::move(other.request_);
      this->response_ = std::move(other.response_);
      return *this;
    }

    Session(const Session& other) = delete;

    Session& operator=(const Session& other) = delete;

    ~Session() = default;

   private:
    // public:
    SessionId session_id_;
    TcpSocketHandle socket_handle_;
    http1::Request request_{};
    http1::Response response_{};
  };

  using ConstBuffer = std::span<const std::byte>;
  using MutableBuffer = std::span< std::byte>;

  // WARN: useless
  inline auto CopyArray(ConstBuffer buffer) noexcept -> std::string {
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

  struct IoResult {
    std::error_code ec{};
    std::size_t bytes_size{0};
  };

  struct SocketRecvMeta {
    TcpSocketHandle socket;
    std::array<std::byte, 8192> recv_buffer{};
    std::size_t unparsed_size{0};
    http1::Request request{};
    http1::RequestParser parser{&request};
    std::error_code ec{};
    std::uint32_t reuse_cnt{0};
  };

  // NOTE: some principle
  // 1. all temporary things use just to hold
  // 2. all long lived things saved in server
  // 3.

  inline auto CreateSessionId() noexcept -> SessionId {
    static uint64_t n = 0;
    return std::to_string(n);
  }

  inline auto CreateSession(TcpSocketHandle socket, http1::Request&& req) noexcept -> Session {
    std::string session_id = CreateSessionId();
    Session s{session_id, socket};
    // s.request_ = std::move(req);
    return s;
  }

  //  request -> response
  inline auto Handle(http1::Request&& request) noexcept -> auto {
    http1::Response response;
    // make response
    response.status_code = http1::HttpStatusCode::kOK;
    response.version = request.version;
    return response;
  }

  struct SocketSendMeta {
    TcpSocketHandle socket;
    std::string start_line_and_headers;
    http1::Response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
    uint32_t send_once_max_wait_time_ms;
  };

  struct HostSocketEnv {
    std::uint32_t reuse_cnt = 0;
    bool need_keep_alive = false;
  };

  // TODO(xiaoming): how to make ipv6?
  class Server {
   public:
    Server(std::string_view addr, uint16_t port) noexcept
      : Server(sio::ip::endpoint{sio::ip::make_address_v4(addr), port}) {
    }

    Server(sio::ip::address addr, uint16_t port) noexcept
      : Server(sio::ip::endpoint{addr, port}) {
    }

    explicit Server(sio::ip::endpoint endpoint) noexcept
      : endpoint_{endpoint}
      , acceptor_{&context_, sio::ip::tcp::v4(), endpoint} {
    }

    void Run() noexcept;

    // just(Request) or just_error(error_code)
    ex::sender auto CreateRequest(SocketRecvMeta meta) noexcept;

    ex::sender auto SendResponse(SocketSendMeta meta) noexcept;

    ex::sender auto SendResponseLineAndHeaders(TcpSocketHandle socket, ConstBuffer buffer);

    ex::sender auto SendResponseBody(TcpSocketHandle socket, ConstBuffer buffer);

    ex::sender auto Alarm(uint32_t milliseconds) noexcept {
      return exec::schedule_after(
        context_.get_scheduler(), std::chrono::milliseconds(milliseconds));
    }

    ex::sender auto RecvOnce(TcpSocketHandle socket, MutableBuffer buffer, uint32_t time_ms);

    ex::sender auto RecvOnce(TcpSocketHandle socket, MutableBuffer buffer);


   private:
    exec::io_uring_context context_{};
    sio::ip::endpoint endpoint_{};
    TcpAcceptor acceptor_;
    std::unordered_map<SessionId, Session> sessions_;
    std::unordered_map<std::string, DomainConfig> domain_configs_;
    SocketRecvConfig server_recv_config_{};
    SocketSendConfig server_send_config_{};
    uint32_t keep_alive_time_ms_{12000};
  };

} // namespace net::tcp
