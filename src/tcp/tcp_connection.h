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

#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "http1/http_response.h"
#include "sio/io_concepts.hpp"
#include "sio/ip/address.hpp"
#include "stdexec/execution.hpp"

namespace net::tcp {

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

  template <class ThenSender, class ElseSender>
  exec::variant_sender<ThenSender, ElseSender>
    if_then_else(bool condition, ThenSender then, ElseSender otherwise) {
    if (condition) {
      return then;
    }
    return otherwise;
  }

  // WARN: useless
  inline auto CopyArray(std::span<std::byte> buffer) noexcept -> std::string {
    std::string ss;
    for (auto b: buffer) {
      ss.push_back(static_cast<char>(b));
    }
    return ss;
  }

  struct RecvMeta {
    TcpSocketHandle socket;
    std::array<std::byte, 8192> recv_buffer{};
    std::size_t unparsed_size{0};
    http1::Request request{};
    http1::RequestParser parser{&request};
  };

  // just(Request) or error
  inline stdexec::sender auto CreateRequest(TcpSocketHandle socket) noexcept {

    auto read_then_parse = [](RecvMeta& recv_meta) noexcept -> stdexec::sender auto {
      // TODO:use subspan
      auto* beg = recv_meta.recv_buffer.begin() + recv_meta.unparsed_size;
      std::size_t buffer_left_size = recv_meta.recv_buffer.size() - recv_meta.unparsed_size;
      return sio::async::read_some(recv_meta.socket, std::span{beg, buffer_left_size})
           | stdexec::let_value([&recv_meta](std::size_t read_size) {
               if (read_size == 0) {
                 return stdexec::just(std::error_code(http1::Error::kEndOfStream));
               }
               std::size_t need_parsed_size = recv_meta.unparsed_size + read_size;
               std::string ss = CopyArray(
                 {recv_meta.recv_buffer.begin(), need_parsed_size}); // DEBUG
               std::cout << ss;                                      // DEBUG
               std::error_code ec{};                                 // DEBUG
               std::size_t parsed_size = recv_meta.parser.Parse(ss, ec);

               if (ec == http1::Error::kNeedMore) {
                 if (parsed_size < need_parsed_size) {
                   // Move unparsed data to the front of buffer.
                   // WARN: this isn't efficient, a new way is needed.
                   std::size_t unparsed_size = need_parsed_size - parsed_size;
                   ::memcpy(
                     recv_meta.recv_buffer.begin(),
                     recv_meta.recv_buffer.begin() + parsed_size,
                     unparsed_size);
                   recv_meta.unparsed_size = unparsed_size;
                 }
               }
               return stdexec::just(ec);
             });
    };

    return stdexec::just(RecvMeta{.socket = socket}) //
         | stdexec::let_value([=](RecvMeta& recv_meta) {
             return read_then_parse(recv_meta) //
                  | stdexec::let_value([](std::error_code& ec) {
                      return if_then_else(
                        ec == http1::Error::kNeedMore,
                        stdexec::just(false),
                        if_then_else(
                          ec == std::errc{}, stdexec::just(true), stdexec::just_error(ec)));
                    })
                  | exec::repeat_effect_until()
                  | stdexec::let_value([&recv_meta]() { return stdexec::just(recv_meta.request); });
           });
  }

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

  struct SendMeta {
    TcpSocketHandle socket;
    std::string send_buffer;
    http1::Response response;
  };

  inline auto SendResponseHeaders(TcpSocketHandle socket, http1::Response& response) noexcept
    -> auto {
    return stdexec::just(SendMeta{.socket = socket, .response = response}) //
         | stdexec::then([](SendMeta&& meta) {
             // Fill response response line and headers to send buffer
             std::string headers = "HTTP/1.1 200 OK\r\nContent-Length:11\r\n\r\n"; // DEBUG
             meta.send_buffer.resize(headers.size());
             ::memcpy(meta.send_buffer.data(), headers.data(), headers.size());
             return meta;
           })
         | stdexec::let_value([](SendMeta& meta) {
             return sio::async::write(meta.socket, std::as_bytes(std::span(meta.send_buffer)));
           });
  }

  inline auto SendResponseBody(TcpSocketHandle socket, http1::Response& response) noexcept {
    return stdexec::just(SendMeta{.socket = socket, .response = response}) //
         | stdexec::let_value([](SendMeta& meta) {
             return sio::async::write(meta.socket, std::as_bytes(std::span(meta.response.Body())));
           });
  }

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

    void Run() noexcept {
      auto accept_connections = sio::async::use_resources(
        [&](TcpAcceptorHandle acceptor_handle) noexcept {
          return sio::async::accept(acceptor_handle)
               | sio::let_value_each([&](TcpSocketHandle socket) {
                   return CreateRequest(socket) //
                        | stdexec::then(Handle) //
                        | stdexec::let_value([socket](http1::Response& response) {
                            return SendResponseHeaders(socket, response)
                                 | stdexec::then([](std::size_t nbytes) {
                                     std::cout << "send bytes: " << nbytes << std::endl;
                                   });
                          }) //
                        | stdexec::upon_error([](auto&& ec) {});
                 })
               | sio::ignore_all();
        },
        acceptor_);

      stdexec::sync_wait(exec::when_any(accept_connections, context_.run()));
    }

    void CreateResponse();

   private:
    exec::io_uring_context context_{};
    sio::ip::endpoint endpoint_{};
    TcpAcceptor acceptor_;
    std::unordered_map<SessionId, Session> sessions_;
  };

} // namespace net::tcp
