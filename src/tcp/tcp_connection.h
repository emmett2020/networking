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

  struct socket_recv_meta {
    std::array<std::byte, 8192> recv_buffer{};
    std::size_t unparsed_size{0};
    http1::Request request{};
    http1::RequestParser parser{&request};
    std::error_code ec{};
    std::uint64_t recv_cnt = 0;
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

  struct SocketSendMeta {
    TcpSocketHandle socket;
    std::string start_line_and_headers;
    http1::Response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
    uint32_t send_once_max_wait_time_ms;
  };

  struct TcpSession {
    TcpSocketHandle socket;
    exec::io_uring_context* ctx{nullptr};
    std::size_t id = 0;
    bool need_keep_alive = false;
    std::size_t keep_alive_time_ms = 12000;
    std::size_t reuse_cnt = 0;
    std::size_t total_send_size = 0;
    std::size_t total_recv_size = 0;
    std::size_t last_recv_size = 0;
    std::size_t last_send_size = 0;
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

    // when milliseconds passed, err will occur.
    // auto alarm(uint64_t milliseconds, http1::Error err) noexcept;

    //  request -> response
    http1::Response&& Handle(http1::Request&& request) noexcept {
      http1::Response response;
      // make response
      response.status_code = http1::HttpStatusCode::kOK;
      response.version = request.version;
      return std::move(response);
    }

    // auto create_request(TcpSession& session) noexcept;

    ex::sender auto SendResponse1(SocketSendMeta meta) noexcept;

    void SendResponse(http1::Response&& res, TcpSession& session) noexcept {
    }

    ex::sender auto SendResponseLineAndHeaders(TcpSocketHandle socket, ConstBuffer buffer);

    ex::sender auto SendResponseBody(TcpSocketHandle socket, ConstBuffer buffer);

    template <class Sender>
    ex::sender auto ExecuteWithTimeout(
      Sender&& task,
      uint32_t timeout_milliseconds,
      http1::Error error) noexcept {
      return ex::when_any(
        std::forward<Sender>(task),
        ex::schedule_after(
          context_.get_scheduler(), std::chrono::milliseconds(timeout_milliseconds))
          | ex::let_value([error] { return ex::just_error(std::error_code(error)); }));
    }

    // auto recv(TcpSocketHandle socket, MutableBuffer buffer, uint32_t time_ms);


    http1::Request&& SetReuseFlag(http1::Request&& request, TcpSession& conn);


    bool UpdateReuseCnt(TcpSession& conn);


    http1::Response&& UpdateSession(http1::Response&& response, TcpSession& connection);

    bool NeedKeepAlive(TcpSession& conn) const {
      return true;
      // return conn.NeedKeepAlive();
    }

    http1::Request&& UpdateRequest(http1::Request&& req);


    http1::Response&& UpdateResponse(http1::Response&& res);

   private:
    exec::io_uring_context context_{};
    sio::ip::endpoint endpoint_{};
    TcpAcceptor acceptor_;
  };

} // namespace net::tcp
