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

#include <unordered_map>

#include <exec/linux/io_uring_context.hpp>
#include <exec/repeat_effect_until.hpp>
#include "sio/io_uring/socket_handle.hpp"

#include "sio/ip/endpoint.hpp"
#include "sio/ip/tcp.hpp"
#include "sio/sequence/ignore_all.hpp"
#include "sio/sequence/let_value_each.hpp"

#include "http1/http_request.h"
#include "http1/http_response.h"

namespace net::tcp {

using TcpSocket = sio::io_uring::socket_handle<sio::ip::tcp>;
using TcpAcceptor = sio::io_uring::acceptor<sio::ip::tcp>;

class TcpConnection {
 public:
 private:
  TcpSocket socket_;
};

// TODO(xiaoming): concept
template <typename Connection, typename Request, typename Response>
class Session {
 public:
 private:
  Connection connection_;
  Request request_;
  Response response_;
};

using TcpSession = Session<TcpConnection, http1::Request, http1::Response>;

using SessionId = std::string;

class Server {
 public:
  explicit Server(sio::ip::endpoint endpoint) noexcept
      : endpoint_{endpoint},
        acceptor_{&context_, sio::ip::tcp::v4(), sio::ip::endpoint{endpoint_}} {
  }

  // Currently just make a echo server.
  void Run() noexcept {}

  void AcceptOne();
  void CreateRequest();
  void CreateResponse();
  void SendResponse();

 private:
  sio::ip::endpoint endpoint_{};
  exec::io_uring_context context_{};
  TcpAcceptor acceptor_;
  std::unordered_map<SessionId, TcpSession> sessions_;
};

}  // namespace net::tcp
