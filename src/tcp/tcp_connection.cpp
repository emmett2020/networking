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
#include "exec/linux/io_uring_context.hpp"
#include "exec/repeat_effect_until.hpp"
#include "exec/timed_scheduler.hpp"
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "http1/http_response.h"
#include "sio/io_concepts.hpp"
#include "stdexec/execution.hpp"
#include "utils/if_then_else.h"
#include <cstdint>
#include <exception>
#include <span>
#include <type_traits>
#include <utility>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

namespace net::tcp {

  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
  using namespace std;     // NOLINT

  // Get detailed error enum by message parser state.
  inline http1::Error
    get_detailed_error_by_state(http1::RequestParser::MessageState state) noexcept {
    using State = http1::RequestParser::MessageState;
    switch (state) {
    case State::kNothingYet:
      return http1::Error::kRecvRequestTimeoutWithNothing;
    case State::kStartLine:
    case State::kExpectingNewline:
      return http1::Error::kRecvRequestLineTimeout;
    case State::kHeader:
      return http1::Error::kRecvRequestHeadersTimeout;
    case State::kBody:
      return http1::Error::kRecvRequestBodyTimeout;
    case State::kCompleted:
      return http1::Error::kSuccess;
    }
  }

  // ctx must be aleady ran
  auto alarm(io_uring_context& ctx, uint64_t milliseconds, http1::Error err) noexcept {
    // Alarm after milliseconds
    return ex::let_value(
      ex::schedule_after(ctx.get_scheduler(), std::chrono::milliseconds(milliseconds)),
      [err] { return ex::just_error(std::error_code{err}); });
  }

  // Receive some bytes then save to buffer.
  // If received size is zero, kEndOfStream is set to downstream pipeline.
  auto recv(io_uring_context& ctx, TcpSocketHandle socket, MutableBuffer buffer, uint32_t time_ms) {
    auto snd = sio::async::read_some(socket, buffer) //
             | ex::let_value([](std::size_t read_bytes_size) noexcept {
                 return ex::if_then_else(
                   read_bytes_size > 0,
                   ex::just(read_bytes_size),
                   ex::just_error(std::error_code(http1::Error::kEndOfStream)));
               });
    return ex::when_any(snd, alarm(ctx, time_ms, http1::Error::kRecvTimeout));
  }

  // Update metrics
  // Need templates constexpr to check whether users need metrics.
  auto update_data(std::size_t recv_size, socket_recv_meta& meta, TcpSession& session) {
    session.total_recv_size += recv_size;
    meta.unparsed_size += recv_size;
    ++meta.recv_cnt;
  }

  std::error_code parse(socket_recv_meta& meta) {
    std::string ss = CopyArray(meta.recv_buffer); // DEBUG
    std::error_code ec{};                         // DEBUG
    std::size_t parsed_size = meta.parser.Parse(ss, ec);
    if (ec == http1::Error::kNeedMore) {
      if (parsed_size < meta.unparsed_size) {
        // WARN: This isn't efficient, a new way is needed.
        //       Move unparsed data to the front of buffer.
        meta.unparsed_size -= parsed_size;
        ::memcpy(
          meta.recv_buffer.begin(), meta.recv_buffer.begin() + parsed_size, meta.unparsed_size);
      }
    }
    return ec;
  }

  inline bool check_parse_done(std::error_code ec) {
    return ec && ec != http1::Error::kNeedMore;
  }

  uint64_t update_recv_time(socket_recv_meta& meta) {
    constexpr std::size_t kTotalRecvTimeout = 120000;
    constexpr std::size_t kPerRecvTimeout = 1200;
    int time_ms = 0;
    if (meta.recv_cnt == 0) {
      time_ms = 12000; // KEEP ALIVE TIME
    } else {
      time_ms = 12001; // std::min(Total left time, per read time);
                       // if per read time not set, use total left time.
    }
    return time_ms;
  }

  auto create_request(TcpSession& session) noexcept {
    std::cout << "Called create request.\n";
    return ex::just(socket_recv_meta{}) //
         | ex::let_value([&](socket_recv_meta& meta) {
             auto update_recv_buffer = [&] {
               return std::span{meta.recv_buffer}.subspan(meta.unparsed_size);
             };

             return recv(*session.ctx, session.socket, update_recv_buffer(), update_recv_time(meta))
                  | ex::then([&](uint64_t recv_size) { update_data(recv_size, meta, session); })
                  | ex::then([&] { return parse(meta); })
                  | ex::then([&](std::error_code ec) { return check_parse_done(ec); })
                  | ex::repeat_effect_until() //
                  | ex::let_value([&] {
                      return ex::if_then_else(
                        meta.parser.State() == http1::RequestParser::MessageState::kCompleted,
                        ex::just(std::move(meta.request)),
                        ex::just_error(get_detailed_error_by_state(meta.parser.State())));
                    });
           });
  }

  ex::sender auto Server::SendResponseLineAndHeaders(TcpSocketHandle socket, ConstBuffer buffer) {
    // auto write = sio::async::write(socket, buffer);
    // auto alarm = Alarm(server_send_config_.send_response_line_and_headers_max_wait_time_ms)
    //            | let_value([] { //
    //                // may replace to write_some to judge the real send data.
    //                return ex::just_error(http1::Error::kSendResponseLineAndHeadersTimeout);
    //              });
    // return ex::when_any(write, alarm);
    return just(1);
  }

  ex::sender auto Server::SendResponseBody(TcpSocketHandle socket, ConstBuffer buffer) {
    // auto write = sio::async::write(socket, buffer);
    // auto alarm = Alarm(server_send_config_.send_body_max_wait_time_ms) //
    //            | let_value([] {                                        //
    //                return ex::just_error(http1::Error::kSendResponseBodyTimeout);
    //              });
    // return ex::when_any(write, alarm);
    return just(1);
  }

  ex::sender auto Server::SendResponse1(SocketSendMeta meta) noexcept {
    return just(1);
    // auto create_response_line_and_headers_buffer = [&meta] {
    //   std::optional<std::string> response_str = meta.response.MakeResponseString();
    //   if (response_str.has_value()) {
    //     meta.start_line_and_headers = std::move(*response_str);
    //   }
    //   return ex::if_then_else(
    //     response_str.has_value(), ex::just(), ex::just_error(http1::Error::kInvalidResponse));
    // };
    //
    // auto update_send_size = [&meta](std::size_t send_bytes_size) {
    //   meta.total_send_size += send_bytes_size;
    //   std::cout << "send bytes: " << meta.total_send_size << std::endl;
    // };
    //
    //
    // auto alarm = Alarm(server_send_config_.send_all_max_wait_time_ms) //
    //            | let_value([] {                                       //
    //                return ex::just_error(http1::Error::kSendTimeout);
    //              });
    //
    // auto send_response =
    //   create_response_line_and_headers_buffer() //
    //   | ex::let_value([this, &meta] {
    //       return SendResponseLineAndHeaders(
    //         meta.socket, std::as_bytes(std::span(meta.start_line_and_headers)));
    //     })
    //   | ex::then(update_send_size) //
    //   | ex::let_value([this, &meta] {
    //       bool need_send_body = true;
    //       return ex::if_then_else(
    //         need_send_body,
    //         SendResponseBody(meta.socket, std::as_bytes(std::span(meta.response.Body()))),
    //         ex::just(0LU));
    //     })
    //   | ex::then(update_send_size);
    //
    // return exec::when_any(send_response, alarm);
  }

  bool NeedKeepAlive(const http1::Request& request) noexcept {
    if (request.ContainsHeader(http1::kHttpHeaderConnection)) {
      // TODO(xiaoming): extract value
      return true;
    }
    if (request.Version() == http1::HttpVersion::kHttp11) {
      return true;
    }
    return false;
  }

  http1::Response&& Server::UpdateSession(http1::Response&& response, TcpSession& connection) {
    // bool need_keep_alive = response.ContainsHeader("Connection")
    // && response.HeaderValue("Connection") == "keep-alive";
    // connection.SetKeepAlive(need_keep_alive);
    return std::move(response);
  }

  bool Server::UpdateReuseCnt(TcpSession& conn) {
    // if (conn.NeedKeepAlive()) {
    // conn.IncreaseReuseCntOnce();
    // }
    return conn.need_keep_alive;
  }

  http1::Request&& UpdateRequest(http1::Request&& req) {
    return std::move(req);
  }

  http1::Response&& UpdateResponse(http1::Response&& res) {
    return std::move(res);
  }

  void Server::Run() noexcept {
    auto handles = sio::async::use_resources(
      [&](TcpAcceptorHandle acceptor_handle) noexcept {
        return sio::async::accept(acceptor_handle)
             | sio::let_value_each([&, this](TcpSocketHandle socket) {
                 std::cout << "accepted one\n";
                 return ex::just(TcpSession{socket, &context_})
                      | ex::let_value([this](TcpSession& session) { //
                          return create_request(session)            //
                               | ex::upon_error([]<class E>(E&& e) noexcept {
                                   if constexpr (std::is_same_v<E, std::error_code>) {
                                     std::cout
                                       << "Error orrcurred: " << std::forward<E>(e).message()
                                       << "\n";
                                   } else if constexpr (std::is_same_v<E, std::exception_ptr>) {
                                     std::cout << "Error orrcurred: exception_ptr\n";
                                   } else {
                                     std::cout << "Unknown Error orrcurred\n";
                                   }
                                 });
                          // ex::sync_wait(std::move(r));
                          // return just(1);
                          // | ex::then_value(this, &Server::UpdateRequest)
                          // | ex::then_value(this, &Server::Handle)
                          // | ex::then_value(this, &Server::UpdateResponse)
                          // | ex::then_value(this, &Server::SendResponse, session)
                          // | ex::then_value(this, &Server::NeedKeepAlive, session)
                          // | ex::repeat_effect_until() //
                          // | ex::upon_error([]<class E >(E&&) {});
                        });
               })
             | sio::ignore_all();
      },
      acceptor_);
    ex::sync_wait(exec::when_any(handles, context_.run()));
  }


} // namespace net::tcp
