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
#include "exec/variant_sender.hpp"
#include "exec/when_any.hpp"
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "http1/http_response.h"
#include "sio/io_concepts.hpp"
#include "stdexec/__detail/__p2300.hpp"
#include "stdexec/execution.hpp"
#include "utils/if_then_else.h"
#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

namespace net::tcp {

  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
  using namespace std;     // NOLINT

  std::error_code GetErrorCodeByRequestParseState(http1::RequestParser& parser) noexcept {
    auto state = parser.State();
    if (state == http1::RequestParser::MessageState::kNothingYet) {
      return http1::Error::kRecvRequestTimeoutWithNothing;
    }

    if (state == http1::RequestParser::MessageState::kStartLine) {
      return http1::Error::kRecvRequestLineTimeout;
    }

    if (state == http1::RequestParser::MessageState::kBody) {
      return http1::Error::kRecvRequestBodyTimeout;
    }
    return http1::Error::kRecvRequestHeadersTimeout;
  }

  ex::sender auto Server::ReadOnce(SocketRecvMeta& meta, uint32_t time_ms) noexcept {
    ex::sender auto read_once = sio::async::read_some(meta.socket, meta.recv_buffer) //
                              | ex::upon_stopped([&meta]() -> std::size_t {
                                  meta.ec = {};
                                  return 0;
                                });

    ex::sender auto alarm = WaitToAlarm(time_ms) | then([] -> std::size_t { return 0; });

    // when_any should be encap to timed_sender
    return exec::when_any(read_once, alarm) //
         | then([&meta](std::size_t read_size) { meta.unparsed_size = read_size; })
         | let_value([&meta]() {
             return if_then_else(
               meta.ec == std::errc{}, ex::just(meta.unparsed_size), ex::just_error(meta.ec));
           });
  }

  // just(Request) or just_error(error_code)
  ex::sender auto Server::CreateRequest(TcpSocketHandle socket, uint32_t reuse_cnt) noexcept {

    auto read_then_parse = [this, reuse_cnt](SocketRecvMeta& meta) noexcept -> ex::sender auto {
      auto free_buffer = std::span{meta.recv_buffer}.subspan(meta.unparsed_size);

      uint32_t first_read_wait_time = 0;
      if (reuse_cnt > 0) { // not first time accept
        first_read_wait_time = keep_alive_time_ms_;
      } else {
        first_read_wait_time = server_recv_config_.recv_once_max_wait_time_ms;
      }

      ex::sender auto read_once = ReadOnce(meta, first_read_wait_time);

      return read_once //
           | ex::let_value([&meta](std::size_t read_size) {
               if (read_size == 0) {
                 return ex::just(std::error_code(http1::Error::kEndOfStream));
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
               return ex::just(ec);
             });
    };

    return ex::just(SocketRecvMeta{.socket = socket}) //
         | ex::let_value([=](SocketRecvMeta& recv_meta) {
             ex::sender auto recv_all_xxx =
               read_then_parse(recv_meta) //
               | ex::let_value([=, &recv_meta](std::error_code& ec) {
                   //
                   ex::sender auto recv_all =
                     read_then_parse(recv_meta) //
                     | ex::let_value([](std::error_code& ec) {
                         return if_then_else(
                           ec == http1::Error::kNeedMore,
                           ex::just(false),
                           if_then_else(ec == std::errc{}, ex::just(true), ex::just_error(ec)));
                       }) //
                     | exec::repeat_effect_until();

                   return if_then_else(
                     ec && ec != http1::Error::kNeedMore,
                     ex::just_error(ec),
                     if_then_else(ec == std::errc{}, ex::just(), recv_all));
                 });

             ex::sender auto alarm = exec::schedule_after(
               context_.get_scheduler(),
               std::chrono::milliseconds(server_recv_config_.recv_all_max_wait_time_ms));

             return exec::when_any(recv_all_xxx, alarm) //
                  | ex::let_value([&recv_meta]() {
                      return if_then_else(
                        recv_meta.parser.State() == http1::RequestParser::MessageState::kCompleted,
                        ex::just(std::move(recv_meta.request)),
                        ex::just_error(GetErrorCodeByRequestParseState(recv_meta.parser)));
                    });
           });
  }

  ex::sender auto Server::SendResponseLineAndHeaders(TcpSocketHandle socket, ConstBuffer buffer) {
    auto write = sio::async::write(socket, buffer) //
               | ex::let_value([](std::size_t send_bytes_size) {
                   return ex::just(IoResult{.bytes_size = send_bytes_size});
                 });
    auto alarm = WaitToAlarm(server_send_config_.send_response_line_and_headers_max_wait_time_ms)
               | let_value([] {
                   return ex::just(IoResult{.ec = http1::Error::kSendResponseHeadersTimeout});
                 });
    return ex::when_any(write, alarm);
  }

  ex::sender auto Server::SendResponseBody(TcpSocketHandle socket, ConstBuffer buffer) {
    auto write = sio::async::write(socket, buffer) //
               | ex::let_value([](std::size_t send_bytes_size) {
                   return ex::just(IoResult{.bytes_size = send_bytes_size});
                 });
    auto alarm = WaitToAlarm(server_send_config_.send_body_max_wait_time_ms) //
               | let_value([] {                                              //
                   return ex::just(IoResult{.ec = http1::Error::kSendResponseBodyTimeout});
                 });
    return ex::when_any(write, alarm);
  }

  ex::sender auto Server::SendResponse(SocketSendMeta meta) noexcept {
    auto create_response_line_and_headers = [&meta] {
      std::optional<std::string> response_str = meta.response.MakeResponseString();
      if (response_str.has_value()) {
        meta.start_line_and_headers = std::move(*response_str);
        meta.ec = {};
      } else {
        meta.ec = http1::Error::kInvalidResponse;
      }
      return just();
    };

    auto assert_meta_error_code_is_clear = [&meta] {
      return ex::let_value([&meta] {
        return if_then_else(meta.ec == std::errc{}, just(), just_error(meta.ec));
      });
    };

    auto fill_error_code_and_send_size_of_meta = [&meta](IoResult io_result) {
      if (!io_result.ec) {
        meta.total_send_size += io_result.bytes_size;
      } else {
        meta.ec = io_result.ec;
      }
      std::cout << "send bytes: " << meta.total_send_size << std::endl;
    };

    ex::sender auto send_response =
      create_response_line_and_headers()  //
      | assert_meta_error_code_is_clear() //
      | ex::let_value([this, &meta] {
          return SendResponseLineAndHeaders(
            meta.socket, std::as_bytes(std::span(meta.start_line_and_headers)));
        })
      | ex::then(fill_error_code_and_send_size_of_meta) //
      | assert_meta_error_code_is_clear()               //
      | ex::let_value([this, &meta] {
          bool need_send_body = true;
          return if_then_else(
            need_send_body,
            SendResponseBody(meta.socket, std::as_bytes(std::span(meta.response.Body()))),
            ex::just(IoResult{}));
        })
      | ex::then(fill_error_code_and_send_size_of_meta) //
      | assert_meta_error_code_is_clear();

    ex::sender auto alarm = WaitToAlarm(server_send_config_.send_all_max_wait_time_ms);

    return exec::when_any(send_response, alarm) //
         | assert_meta_error_code_is_clear();
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

  void Server::Run() noexcept {
    auto accept_connections = sio::async::use_resources(
      [&](TcpAcceptorHandle acceptor_handle) noexcept {
        return sio::async::accept(acceptor_handle)
             | sio::let_value_each([&, this](TcpSocketHandle socket) {
                 // run_once
                 return ex::just(HostSocketEnv{}) //
                      | let_value([this, socket](HostSocketEnv& socket_env) {
                          return CreateRequest(socket, socket_env.reuse_cnt) //
                               | ex::then([&socket_env](http1::Request&& request) {
                                   // lambda?
                                   socket_env.need_keep_alive = NeedKeepAlive(request);
                                   return std::move(request);
                                 })
                               | ex::then(Handle) //
                               | ex::let_value([this, socket](http1::Response& response) {
                                   return SendResponse(SocketSendMeta{
                                     .socket = socket, .response = std::move(response)});
                                 })
                               | ex::then([this, &socket_env] {
                                   bool keep_alive = true;
                                   if (keep_alive) {
                                     ++socket_env.reuse_cnt;
                                   }
                                   return keep_alive;
                                 })
                               | exec::repeat_effect_until() //
                               | ex::upon_error([]<class E >(E&&) {});
                        });
               })
             | sio::ignore_all();
      },
      acceptor_);

    ex::sync_wait(exec::when_any(accept_connections, context_.run()));
  }
} // namespace net::tcp
