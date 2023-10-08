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

#include "tcp_connection.h"
#include "exec/timed_scheduler.hpp"
#include "exec/when_any.hpp"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "sio/io_concepts.hpp"
#include "stdexec/__detail/__p2300.hpp"
#include "stdexec/execution.hpp"
#include "utils/if_then_else.h"
#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>

namespace net::tcp {

  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
  using namespace std;     // NOLINT

  std::error_code GetErrorCodeByRequestParseState(http1::RequestParser& parser) noexcept {
    auto state = parser.State();
    if (state == http1::RequestParser::MessageState::kNothingYet) {
      return http1::Error::kRecvTimeoutWithReceivingNothing;
    }

    if (state == http1::RequestParser::MessageState::kStartLine) {
      return http1::Error::kRecvRequestLineTimeout;
    }

    if (state == http1::RequestParser::MessageState::kBody) {
      return http1::Error::kRecvRequestBodyTimeout;
    }
    return http1::Error::kRecvRequestHeadersTimeout;
  }

  stdexec::sender auto Server::ReadOnce(SocketRecvMeta& meta, uint32_t time_ms) noexcept {

    stdexec::sender auto read_once =
      sio::async::read_some(meta.socket, meta.recv_buffer) //
      | stdexec::upon_stopped([&meta]() -> std::size_t {
          // error_code Error::KeepAlive
          meta.ec = {};
          return 0;
        });

    stdexec::sender auto alarm = WaitToAlarm(time_ms) | then([] -> std::size_t { return 0; });

    // when_any should be encap to timed_sender
    return exec::when_any(read_once, alarm) //
         | then([&meta](std::size_t read_size) { meta.unparsed_size = read_size; })
         | let_value([&meta]() {
             return if_then_else(
               meta.ec == std::errc{},
               stdexec::just(meta.unparsed_size),
               stdexec::just_error(meta.ec));
           });
  }

  // just(Request) or just_error(error_code)
  stdexec::sender auto Server::CreateRequest(TcpSocketHandle socket) noexcept {

    auto read_then_parse = [this](SocketRecvMeta& meta) noexcept -> stdexec::sender auto {
      auto free_buffer = std::span{meta.recv_buffer}.subspan(meta.unparsed_size);

      bool need_keep_alive = true; // DEBUG
      uint32_t first_read_wait_time = 0;
      if (need_keep_alive) {
        first_read_wait_time = keep_alive_time_ms_;
      } else {
        first_read_wait_time = server_recv_config_.recv_once_max_wait_time_ms;
      }

      stdexec::sender auto read_once = ReadOnce(meta, first_read_wait_time);

      // return sio::async::read_some(meta.socket, free_buffer)
      return read_once | stdexec::let_value([&meta](std::size_t read_size) {
               if (read_size == 0) {
                 return stdexec::just(std::error_code(http1::Error::kEndOfStream));
               }
               std::size_t need_parsed_size = meta.unparsed_size + read_size;
               std::string ss = CopyArray({meta.recv_buffer.begin(), need_parsed_size}); // DEBUG
               std::cout << ss;                                                          // DEBUG
               std::error_code ec{};                                                     // DEBUG
               std::size_t parsed_size = meta.parser.Parse(ss, ec);

               if (ec == http1::Error::kNeedMore) {
                 if (parsed_size < need_parsed_size) {
                   // Move unparsed data to the front of buffer.
                   // WARN: this isn't efficient, a new way is needed.
                   std::size_t unparsed_size = need_parsed_size - parsed_size;
                   ::memcpy(
                     meta.recv_buffer.begin(),
                     meta.recv_buffer.begin() + parsed_size,
                     unparsed_size);
                   meta.unparsed_size = unparsed_size;
                 }
               }
               return stdexec::just(ec);
             });
    };

    return stdexec::just(SocketRecvMeta{.socket = socket}) //
         | stdexec::let_value([=](SocketRecvMeta& recv_meta) {
             stdexec::sender auto recv_all_xxx =
               read_then_parse(recv_meta) //
               | stdexec::let_value([=, &recv_meta](std::error_code& ec) {
                   //
                   stdexec::sender auto recv_all =
                     read_then_parse(recv_meta) //
                     | stdexec::let_value([](std::error_code& ec) {
                         return if_then_else(
                           ec == http1::Error::kNeedMore,
                           stdexec::just(false),
                           if_then_else(
                             ec == std::errc{}, stdexec::just(true), stdexec::just_error(ec)));
                       }) //
                     | exec::repeat_effect_until();

                   return if_then_else(
                     ec && ec != http1::Error::kNeedMore,
                     stdexec::just_error(ec),
                     if_then_else(ec == std::errc{}, stdexec::just(), recv_all));
                 });


             //
             // stdexec::sender auto recv_all =
             //   read_then_parse(recv_meta) //
             //   | stdexec::let_value([](std::error_code& ec) {
             //       return if_then_else(
             //         ec == http1::Error::kNeedMore,
             //         stdexec::just(false),
             //         if_then_else(ec == std::errc{}, stdexec::just(true), stdexec::just_error(ec)));
             //     }) //
             //   | exec::repeat_effect_until();
             //

             stdexec::sender auto alarm = exec::schedule_after(
               context_.get_scheduler(),
               std::chrono::milliseconds(server_recv_config_.recv_all_max_wait_time_ms));

             return exec::when_any(recv_all_xxx, alarm) //
                  | stdexec::let_value([&recv_meta]() {
                      return if_then_else(
                        recv_meta.parser.State() == http1::RequestParser::MessageState::kCompleted,
                        stdexec::just(std::move(recv_meta.request)),
                        stdexec::just_error(GetErrorCodeByRequestParseState(recv_meta.parser)));
                    });
           });
  }

  auto Server::SendResponseHeaders(TcpSocketHandle socket, http1::Response& response) noexcept {
    return stdexec::just(SocketSendMeta{.socket = socket, .response = response}) //
         | stdexec::then([](SocketSendMeta&& meta) {
             // Fill response response line and headers to send buffer
             std::string headers = "HTTP/1.1 200 OK\r\nContent-Length:11\r\n\r\n"; // DEBUG
             meta.send_buffer.resize(headers.size());
             ::memcpy(meta.send_buffer.data(), headers.data(), headers.size());
             return meta;
           })
         | stdexec::let_value([](SocketSendMeta& meta) {
             return sio::async::write(meta.socket, std::as_bytes(std::span(meta.send_buffer)));
           });
  }

  auto Server::SendResponseBody(TcpSocketHandle socket, http1::Response& response) noexcept {
    return stdexec::just(SocketSendMeta{.socket = socket, .response = response}) //
         | stdexec::let_value([](SocketSendMeta& meta) {
             return sio::async::write(meta.socket, std::as_bytes(std::span(meta.response.Body())));
           });
  }

  stdexec::sender auto Server::SendResponse(SocketSendMeta meta) noexcept {
    stdexec::sender auto send_all =
      SendResponseHeaders(meta.socket, meta.response) //
      | stdexec::then([this, &meta](std::size_t nbytes) {
          meta.total_send_size += nbytes;
          std::cout << "send header bytes: " << nbytes << std::endl;
        })
      | stdexec::let_value([this, &meta]() {
          bool need_send_body = true;
          return if_then_else(
            need_send_body, stdexec::just(0), SendResponseBody(meta.socket, meta.response));
        })
      | stdexec::then([&meta](std::size_t nbytes) {
          meta.total_send_size += nbytes;
          std::cout << "send body bytes: " << nbytes << std::endl;
        });

    return exec::when_any(send_all, WaitToAlarm(server_send_config_.send_all_max_wait_time_ms)) //
         | stdexec::let_value([&meta]() {
             return if_then_else(
               meta.ec == std::errc{},
               stdexec::just(meta.total_send_size),
               stdexec::just_error(meta.ec));
           });
  }

  void Server::Run() noexcept {
    auto accept_connections = sio::async::use_resources(
      [&](TcpAcceptorHandle acceptor_handle) noexcept {
        return sio::async::accept(acceptor_handle)
             | sio::let_value_each([&, this](TcpSocketHandle socket) {
                 return CreateRequest(socket) //
                      | stdexec::then(Handle) //
                      | stdexec::let_value([this, socket](http1::Response& response) {
                          return SendResponse(
                            SocketSendMeta{.socket = socket, .response = std::move(response)});
                        })
                      | stdexec::then([this](uint32_t) {
                          bool keep_alive = true;
                          // record not first time request
                          return keep_alive;
                        })
                      | exec::repeat_effect_until() //
                      | stdexec::upon_error([]<class E >(E&&) {});
               })
             | sio::ignore_all();
      },
      acceptor_);

    stdexec::sync_wait(exec::when_any(accept_connections, context_.run()));
  }
} // namespace net::tcp
