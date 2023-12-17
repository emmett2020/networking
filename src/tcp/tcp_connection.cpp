// /*
//  * Copyright (c) 2023 Xiaoming Zhang
//  *
//  * Licensed under the Apache License Version 2.0 with LLVM Exceptions
//  * (the "License"); you may not use this file except in compliance with
//  * the License. You may obtain a copy of the License at
//  *
//  *   https://llvm.org/LICENSE.txt
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */
//
// #include "tcp_connection.h"
// #include "exec/repeat_effect_until.hpp"
// #include "exec/timed_scheduler.hpp"
// #include "exec/variant_sender.hpp"
// #include "exec/when_any.hpp"
// #include "http1/http_common.h"
// #include "http1/http_error.h"
// #include "http1/http_message_parser.h"
// #include "http1/http_request.h"
// #include "http1/http_response.h"
// #include "sio/io_concepts.hpp"
// #include "stdexec/__detail/__p2300.hpp"
// #include "stdexec/execution.hpp"
// #include "utils/if_then_else.h"
// #include "utils/then_call.h"
// #include <chrono>
// #include <cstdint>
// #include <span>
// #include <fcntl.h>
// #include <netinet/in.h>
// #include <unistd.h>
//
// namespace net::tcp {
//
//   using namespace stdexec; // NOLINT
//   using namespace exec;    // NOLINT
//   using namespace std;     // NOLINT
//
//   std::error_code GetErrorCodeByRequestParseState(http1::RequestParser& parser) noexcept {
//     auto state = parser.State();
//     if (state == http1::RequestParser::MessageState::kNothingYet) {
//       return http1::Error::kRecvRequestTimeoutWithNothing;
//     }
//
//     if (state == http1::RequestParser::MessageState::kStartLine) {
//       return http1::Error::kRecvRequestLineTimeout;
//     }
//
//     if (state == http1::RequestParser::MessageState::kBody) {
//       return http1::Error::kRecvRequestBodyTimeout;
//     }
//     return http1::Error::kRecvRequestHeadersTimeout;
//   }
//
//   ex::sender auto Server::RecvOnce(TcpSocketHandle socket, MutableBuffer buffer, uint32_t time_ms) {
//     auto read = sio::async::read_some(socket, buffer) //
//               | let_value([](std::size_t read_bytes_size) {
//                   return if_then_else(
//                     read_bytes_size > 0,
//                     ex::just(read_bytes_size),
//                     ex::just_error(std::error_code(http1::Error::kEndOfStream)));
//                 });
//
//     auto alarm = Alarm(time_ms) //
//                | let_value([] { //
//                    return ex::just_error(std::error_code(http1::Error::kRecvTimeout));
//                  });
//
//     return ex::when_any(read, alarm);
//   }
//
//   ex::sender auto Server::RecvOnce(TcpSocketHandle socket, MutableBuffer buffer) {
//     return sio::async::read_some(socket, buffer) //
//          | let_value([](std::size_t read_bytes_size) {
//              return if_then_else(
//                read_bytes_size > 0,
//                ex::just(read_bytes_size),
//                ex::just_error(std::error_code(http1::Error::kEndOfStream)));
//            });
//   }
//
//   ex::sender auto Server::CreateRequest(SocketRecvMeta meta) noexcept {
//     auto parse_once = [&meta] {
//       std::string ss = CopyArray(meta.recv_buffer); // DEBUG
//       std::error_code ec{};                         // DEBUG
//       std::size_t parsed_size = meta.parser.Parse(ss, ec);
//       if (ec == http1::Error::kNeedMore) {
//         if (parsed_size < meta.unparsed_size) {
//           // WARN: This isn't efficient, a new way is needed.
//           //       Move unparsed data to the front of buffer.
//           meta.unparsed_size -= parsed_size;
//           ::memcpy(
//             meta.recv_buffer.begin(), meta.recv_buffer.begin() + parsed_size, meta.unparsed_size);
//         }
//       }
//
//       return if_then_else(
//         ec && ec != http1::Error::kNeedMore, just_error(ec), just(ec == std::errc{}));
//     };
//
//
//     auto recv_once_no_more_than_keep_alive_time = [this, parse_once, &meta] {
//       auto free_buffer = std::span{meta.recv_buffer}.subspan(meta.unparsed_size);
//       return RecvOnce(meta.socket, std::as_writable_bytes(free_buffer), keep_alive_time_ms_)
//            | ex::then([&meta](std::size_t read_size) { meta.unparsed_size += read_size; });
//     };
//
//     auto recv_once = [this, parse_once, &meta] {
//       auto free_buffer = std::span{meta.recv_buffer}.subspan(meta.unparsed_size);
//       return RecvOnce(meta.socket, std::as_writable_bytes(free_buffer))
//            | ex::then([&meta](std::size_t read_size) { meta.unparsed_size += read_size; });
//     };
//
//     auto alarm = Alarm(server_recv_config_.recv_all_max_wait_time_ms);
//     auto recv_parse_all =
//       recv_once_no_more_than_keep_alive_time() //
//       | ex::let_value(parse_once)              //
//       | ex::let_value([recv_once, parse_once, &meta](bool parse_done) {
//           auto repeat_recv_parse = recv_once()               //
//                                  | ex::let_value(parse_once) //
//                                  | ex::repeat_effect_until();
//           return if_then_else(parse_done, ex::just(), repeat_recv_parse);
//         });
//
//     return ex::when_any(recv_parse_all, alarm) //
//          | ex::let_value([&meta] {
//              return if_then_else(
//                meta.parser.State() == http1::RequestParser::MessageState::kCompleted,
//                ex::just(std::move(meta.request)),
//                ex::just_error(GetErrorCodeByRequestParseState(meta.parser)));
//            });
//   }
//
//   ex::sender auto Server::SendResponseLineAndHeaders(TcpSocketHandle socket, ConstBuffer buffer) {
//     auto write = sio::async::write(socket, buffer);
//     auto alarm = Alarm(server_send_config_.send_response_line_and_headers_max_wait_time_ms)
//                | let_value([] { //
//                    // may replace to write_some to judge the real send data.
//                    return ex::just_error(http1::Error::kSendResponseLineAndHeadersTimeout);
//                  });
//     return ex::when_any(write, alarm);
//   }
//
//   ex::sender auto Server::SendResponseBody(TcpSocketHandle socket, ConstBuffer buffer) {
//     auto write = sio::async::write(socket, buffer);
//     auto alarm = Alarm(server_send_config_.send_body_max_wait_time_ms) //
//                | let_value([] {                                        //
//                    return ex::just_error(http1::Error::kSendResponseBodyTimeout);
//                  });
//     return ex::when_any(write, alarm);
//   }
//
//   ex::sender auto Server::SendResponse(SocketSendMeta meta) noexcept {
//     auto create_response_line_and_headers_buffer = [&meta] {
//       std::optional<std::string> response_str = meta.response.MakeResponseString();
//       if (response_str.has_value()) {
//         meta.start_line_and_headers = std::move(*response_str);
//       }
//       return if_then_else(
//         response_str.has_value(), ex::just(), ex::just_error(http1::Error::kInvalidResponse));
//     };
//
//     auto update_send_size = [&meta](std::size_t send_bytes_size) {
//       meta.total_send_size += send_bytes_size;
//       std::cout << "send bytes: " << meta.total_send_size << std::endl;
//     };
//
//
//     auto alarm = Alarm(server_send_config_.send_all_max_wait_time_ms) //
//                | let_value([] {                                       //
//                    return ex::just_error(http1::Error::kSendTimeout);
//                  });
//
//     auto send_response =
//       create_response_line_and_headers_buffer() //
//       | ex::let_value([this, &meta] {
//           return SendResponseLineAndHeaders(
//             meta.socket, std::as_bytes(std::span(meta.start_line_and_headers)));
//         })
//       | ex::then(update_send_size) //
//       | ex::let_value([this, &meta] {
//           bool need_send_body = true;
//           return if_then_else(
//             need_send_body,
//             SendResponseBody(meta.socket, std::as_bytes(std::span(meta.response.Body()))),
//             ex::just(0LU));
//         })
//       | ex::then(update_send_size);
//
//     return exec::when_any(send_response, alarm);
//   }
//
//   bool NeedKeepAlive(const http1::Request& request) noexcept {
//     if (request.ContainsHeader(http1::kHttpHeaderConnection)) {
//       // TODO(xiaoming): extract value
//       return true;
//     }
//     if (request.Version() == http1::HttpVersion::kHttp11) {
//       return true;
//     }
//     return false;
//   }
//
//   http1::Request Server::SetReuseFlag(http1::Request&& request, HostSocketEnv& socket_env) {
//     socket_env.need_keep_alive = NeedKeepAlive(request);
//     return request;
//   }
//
//   void Server::Run() noexcept {
//     Server server{sio::ip::endpoint{}};
//     auto accept_connections = sio::async::use_resources(
//       [&](TcpAcceptorHandle acceptor_handle) noexcept {
//         return sio::async::accept(acceptor_handle)
//              | sio::let_value_each([&, this](TcpSocketHandle socket) {
//                  // run_once
//                  return ex::just(HostSocketEnv{}) //
//                       | let_value([this, socket, &server](HostSocketEnv& socket_env) {
//                           return CreateRequest(SocketRecvMeta{
//                                    .socket = socket, .reuse_cnt = socket_env.reuse_cnt}) //
//                                | ex::then([&socket_env](http1::Request&& request) {
//                                    // lambda?
//                                    socket_env.need_keep_alive = NeedKeepAlive(request);
//                                    return std::move(request);
//                                  })
//                                | then_call(&server, &Server::SetReuseFlag, socket_env)
//                                | ex::then(Handle) //
//                                | ex::let_value([this, socket](http1::Response& response) {
//                                    return SendResponse(SocketSendMeta{
//                                      .socket = socket, .response = std::move(response)});
//                                  })
//                                | ex::then([this, &socket_env] {
//                                    if (socket_env.need_keep_alive) {
//                                      ++socket_env.reuse_cnt;
//                                    }
//                                    return socket_env.need_keep_alive;
//                                  })
//                                | exec::repeat_effect_until() //
//                                | ex::upon_error([]<class E >(E&&) {});
//                         });
//                })
//              | sio::ignore_all();
//       },
//       acceptor_);
//
//     ex::sync_wait(exec::when_any(accept_connections, context_.run()));
//   }
// } // namespace net::tcp
