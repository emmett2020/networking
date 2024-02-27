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
#include "exec/timed_scheduler.hpp"
#include "exec/variant_sender.hpp"
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "http1/http_response.h"
#include "sio/io_concepts.hpp"
#include "stdexec/execution.hpp"
#include "utils/if_then_else.h"
#include <chrono>
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
  inline http1::Error detailed_error(http1::RequestParser::MessageState state) noexcept {
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

  inline void initialize_remaning_time(socket_recv_handle& handle) {
    if (handle.opt.keepalive_timeout != unlimited_timeout) {
      handle.remaining_timeout = handle.opt.keepalive_timeout;
    } else {
      handle.remaining_timeout = handle.opt.total_timeout;
    }
  }

  // TODO(xiaoming): move timepoint to the front?
  auto update_time_metric(recv_option::duration dur, socket_recv_handle& handle) {
    if (handle.metric.recv_cnt == 0) {
      handle.metric.start_timestamp = std::chrono::system_clock::now();
    }
    handle.metric.end_timestamp = std::chrono::system_clock::now();
    handle.metric.io_duration += dur;
    ++handle.metric.recv_cnt;
  }

  // Update metrics
  auto update_size_metric(std::size_t recv_size, socket_recv_handle& handle) {
    handle.unparsed_size += recv_size;
    handle.metric.total_size += recv_size;
  }

  // Receive some bytes then save to buffer.
  // If received size is zero, kEndOfStream is set to downstream pipeline.
  // return received size and update time.
  auto recv_some(tcp_socket_handle socket, mutable_buffer buffer, recv_option::duration timeout) {
    auto start = std::chrono::system_clock::now();
    auto snd = sio::async::read_some(socket, buffer) //
             | ex::let_value([start](std::size_t size) noexcept {
                 using variant = ex::variant_sender<
                   decltype(ex::just(0LU)),
                   decltype(ex::just_error(http1::Error::kEndOfStream))>;
                 if (size == 0) {
                   return variant(ex::just_error(http1::Error::kEndOfStream));
                 }
                 auto io_duration = std::chrono::duration_cast<recv_option::duration>(
                   std::chrono::system_clock::now() - start);
                 return variant(ex::just(size));
               });
    return ex::when_any(
      snd,
      ex::schedule_after(socket.context_->get_scheduler(), timeout) //
        | ex::let_error([] { return ex::just_error(http1::Error::kRecvTimeout); }));
  }

  std::error_code parse(socket_recv_handle& meta) {
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


  template <class Option>
  concept option = requires(Option& opt) {
    { opt.total_size };
  };

  // auto recv_request(tcp_socket_handle socket, const recv_option& opt) noexcept {
  //   std::cout << "Called create request.\n";
  //   return ex::just(socket_recv_handle{.socket = socket}) //
  //        | ex::let_value([&](socket_recv_handle& handle) {
  //            handle.opt = opt;
  //            auto update_recv_buffer = [&] {
  //              return std::span{handle.recv_buffer}.subspan(handle.unparsed_size);
  //            };
  //            initialize_remaning_time(handle);
  //            return recv_some(handle.socket, update_recv_buffer(), handle.remaining_timeout) //
  //                 | ex::then([&](std::size_t recv_size, recv_option::duration dur) {
  //                     update_data(recv_size, dur, handle);
  //                     return check_parse_done(parse(handle));
  //                   })
  //                 | ex::repeat_effect_until() //
  //                 | ex::let_value([&] {
  //                     return ex::if_then_else(
  //                       handle.parser.State() == http1::RequestParser::MessageState::kCompleted,
  //                       ex::just(std::move(handle.request), handle.metric),
  //                       ex::just_error(detailed_error(handle.parser.State())));
  //                   });
  //          });
  // }

  // recv_option: per request option. so not constexpr.
  // auto recv_request(tcp_socket_handle socket, recv_option opt) noexcept {
  //   std::cout << "Called create request.\n";
  //   return ex::just(socket_recv_handle{}) //
  //        | ex::let_value([&](socket_recv_handle& handle) {
  //            handle.opt = opt;
  //            auto update_recv_buffer = [&] {
  //              return std::span{handle.recv_buffer}.subspan(handle.unparsed_size);
  //            };
  //
  //            return recv_some_timeout(socket, update_recv_buffer(), 11)
  //                 | ex::then([&](uint64_t recv_size, uint64_t duration) {
  //                     update_data(recv_size, duration, handle);
  //                     return check_parse_done(parse(handle));
  //                   })
  //                 | ex::repeat_effect_until() //
  //                 | ex::let_value([&] {
  //                     return ex::if_then_else(
  //                       handle.parser.State() == http1::RequestParser::MessageState::kCompleted,
  //                       ex::just(std::move(handle.request), handle.metric),
  //                       ex::just_error(get_detailed_error_by_state(handle.parser.State())));
  //                   });
  //          });
  // }

  // auto recv_request(TcpSession& session) noexcept {
  //   std::cout << "Called create request.\n";
  //   return ex::just(socket_recv_meta{}) //
  //        | ex::let_value([&](socket_recv_meta& meta) {
  //            auto update_recv_buffer = [&] {
  //              return std::span{meta.recv_buffer}.subspan(meta.unparsed_size);
  //            };
  //
  //            return recv(*session.ctx, session.socket, update_recv_buffer(), update_recv_time(meta))
  //                 | ex::then([&](uint64_t recv_size) { update_data(recv_size, meta, session); })
  //                 | ex::then([&] { return parse(meta); })
  //                 | ex::then([&](std::error_code ec) { return check_parse_done(ec); })
  //                 | ex::repeat_effect_until() //
  //                 | ex::let_value([&] {
  //                     return ex::if_then_else(
  //                       meta.parser.State() == http1::RequestParser::MessageState::kCompleted,
  //                       ex::just(std::move(meta.request)),
  //                       ex::just_error(get_detailed_error_by_state(meta.parser.State())));
  //                   });
  //          });
  // }

  ex::sender auto handle_request(http1::Request& request) noexcept {
    http1::Response response;
    // make response
    response.status_code = http1::HttpStatusCode::kOK;
    response.version = std::move(request).version;
    return ex::just(response);
  }

  ex::sender auto send(
    io_uring_context& ctx,
    tcp_socket_handle socket,
    const_buffer buffer,
    uint64_t time_ms,
    http1::Error err) {
    auto write = sio::async::write(socket, buffer);
    return ex::when_any(write, alarm(ctx, time_ms, err));
  }

  ex::sender auto create_response(SocketSendMeta& meta) {
    std::optional<std::string> response_str = meta.response.MakeResponseString();
    if (response_str.has_value()) {
      meta.start_line_and_headers = std::move(*response_str);
    }
    return ex::if_then_else(
      response_str.has_value(), ex::just(), ex::just_error(http1::Error::kInvalidResponse));
  }

  // Update metrics
  // Need templates constexpr to check whether users need metrics.
  auto update_data(
    std::size_t send_size,
    SocketSendMeta& meta,
    [[maybe_unused]] tcp_session& session) {
    meta.total_send_size += send_size;
    std::cout << "send bytes: " << meta.total_send_size << "\n";
  }

  send_metric
    send_response(http1::Response& response, tcp_socket_handle socket, send_option opt) noexcept {
    return send_metric{};
  }

  ex::sender auto
    SendResponse(io_uring_context& ctx, SocketSendMeta meta, tcp_session& session) noexcept {
    auto send_response =
      create_response(meta) //
      | ex::let_value([&] {
          return send(
            ctx,
            meta.socket,
            std::as_bytes(std::span(meta.start_line_and_headers)),
            10000,
            http1::Error::kSendResponseLineAndHeadersTimeout);
        })
      | ex::then([&](std::size_t send_size) { return update_data(send_size, meta, session); }) //
      | ex::let_value([&] {
          bool need_send_body = true;
          return ex::if_then_else(
            need_send_body,
            send(
              ctx,
              meta.socket,
              std::as_bytes(std::span(meta.response.Body())),
              10000,
              http1::Error::kSendResponseLineAndHeadersTimeout),
            ex::just(0LU));
        })
      | ex::then([&](std::size_t send_size) { return update_data(send_size, meta, session); });
    return send_response;
  }

  bool check_keepalive(const http1::Request& request) noexcept {
    if (request.ContainsHeader(http1::kHttpHeaderConnection)) {
      return true;
    }
    if (request.Version() == http1::HttpVersion::kHttp11) {
      return true;
    }
    return false;
  }

  // http1::Response&& Server::UpdateSession(http1::Response&& response, TcpSession& connection) {
  //   // bool need_keep_alive = response.ContainsHeader("Connection")
  //   // && response.HeaderValue("Connection") == "keep-alive";
  //   // connection.SetKeepAlive(need_keep_alive);
  //   return std::move(response);
  // }

  tcp_session create_session(tcp_socket_handle socket) {
    return tcp_session{socket};
  }

  void update_server_metrics(server_metric server, recv_metric r) {
  }

  void update_server_metrics(server_metric server, send_metric s) {
  }

  template <class E>
  auto handle_error(E&& e) {
    if constexpr (std::is_same_v<E, std::error_code>) {
      std::cout << "Error orrcurred: " << std::forward<E>(e).message() << "\n";
    } else if constexpr (std::is_same_v<E, std::exception_ptr>) {
      std::cout << "Error orrcurred: exception_ptr\n";
    } else {
      std::cout << "Unknown Error orrcurred\n";
    }
  }

  void update_session(tcp_session& session) noexcept {
  }

  void start_server(Server& server) noexcept {
    auto handles = sio::async::use_resources(
      [&](tcp_acceptor_handle acceptor_handle) noexcept {
        return sio::async::accept(acceptor_handle)
             | sio::let_value_each([&](tcp_socket_handle socket) {
                 // std::cout << "accepted one\n";
                 return ex::just(create_session(socket))          //
                      | ex::let_value([&](tcp_session& session) { //
                          return recv_request(session.socket, server.recv_opt)
                               | ex::let_value([&](http1::Request& req, recv_metric metrics) {
                                   session.request = req;
                                   server.update_metric(metrics);
                                   return handle_request(req);
                                 })
                               | ex::then([&](http1::Response&& rsp) {
                                   session.response = std::move(rsp);
                                   auto metrics = send_response(
                                     session.response, session.socket, server.send_opt);
                                   server.update_metric(metrics);
                                 })
                               | ex::then([&] { return check_keepalive(session.request); })
                               | ex::repeat_effect_until() //
                               | ex::then([&]() { update_session(session); })
                               | ex::upon_error([]<class E>(E&& e) {
                                   return handle_error(std::forward<E>(e));
                                 });
                        });
               })
             | sio::ignore_all();
      },
      server.acceptor);
    ex::sync_wait(exec::when_any(handles, server.context.run()));
  }


} // namespace net::tcp

/*
 * template<net::server Server>
 * net::http::start_server(Server& server)
 *     let_value_with(socket)
 *         return handle_accepetd(socket)
 *                | let_value(create_session(socket))
 *                    return prepare_recv()
 *                           | recv_request(socket)
 *                           | update_metrics()
 *                           | handle_request()
 *                           | send_response()
 *                           | update_metrics()
 *                           | check_keepalive()
 *                           | repeat_effect_until()
 *                           | update_session()
 *                           | handle_error();
 *
 *
 *  start_server(server)                           -> void
 *  handle_accepetd(socket)                        -> void
 *  create_session(socket)                         -> tcp::session
 *  prepare_recv(&session)                         -> void
 *  recv_request(socket, recv_option)              -> sender<request, recv_metrics>
 *  handle_request(&request)                       -> sender<response>
 *  send_response(response, socket, send_option)   -> sender<send_metrics>
 *  update_metrics(&metrics)                       -> void
 *
 *  session: 
 *
 *
 *
 */
