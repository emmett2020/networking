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

#include "http1/http_server.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <string>
#include <exception>
#include <type_traits>
#include <utility>
#include <iostream>

#include <exec/repeat_effect_until.hpp>
#include <sio/sequence/ignore_all.hpp>
#include <sio/io_uring/socket_handle.hpp>
#include <sio/ip/tcp.hpp>

#include "utils/timeout.h"
#include "utils/if_then_else.h"
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"
#include "http1/http_response.h"
#include "http1/http_concept.h"

namespace ex {
  using namespace stdexec; // NOLINT
  using namespace exec;    // NOLINT
}

namespace net::http1 {
  using tcp_socket = sio::io_uring::socket_handle<sio::ip::tcp>;
  using tcp_acceptor_handle = sio::io_uring::acceptor_handle<sio::ip::tcp>;
  using tcp_acceptor = sio::io_uring::acceptor<sio::ip::tcp>;
  using const_buffer = std::span<const std::byte>;
  using mutable_buffer = std::span<std::byte>;

  struct send_state {
    tcp_socket socket;
    std::string start_line_and_headers;
    http1::Response response;
    std::error_code ec{};
    std::uint32_t total_send_size{0};
  };

  // tcp session.
  struct session {
    explicit session(tcp_socket socket)
      : socket(socket)
      , id(make_session_id())
      , context(socket.context_) {
    }

    using session_id = int64_t;

    static session_id make_session_id() noexcept {
      static std::atomic_int64_t session_idx{0};
      ++session_idx;
      return session_idx.load();
    }

    exec::io_uring_context* context{nullptr};
    session_id id = 0;
    tcp_socket socket;
    http1::client_request request{};
    http1::Response response{};
    std::size_t reuse_cnt = 0;
  };

  // TODO(xiaoming): how to make ipv6?
  struct server {
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

    exec::io_uring_context context{};
    sio::ip::endpoint endpoint{};
    tcp_acceptor acceptor;
  };

  namespace _parse_request {
    inline bool check_parse_done(std::error_code ec) {
      return ec && ec != http1::Error::kNeedMore;
    }

    struct parse_request_t {
      // Parse a request usually a http client request.
      template <http_request Request>
      stdexec::sender auto operator()(
        MessageParser<Request>& parser,
        mutable_buffer buffer,
        std::size_t unparsed_size) const {
        auto copy_array = [](const_buffer buffer) noexcept -> std::string {
          std::string ss;
          for (auto b: buffer) {
            ss.push_back(static_cast<char>(b));
          }
          return ss;
        };
        std::string ss = copy_array(buffer); // DEBUG
        std::error_code ec{};                // DEBUG
        std::size_t parsed_size = parser.Parse(ss, ec);
        if (ec == http1::Error::kNeedMore) {
          if (parsed_size < unparsed_size) {
            // WARN: This isn't efficient, a new way is needed.
            //       Move unparsed data to the front of buffer.
            unparsed_size -= parsed_size;
            // ::memcpy(buffer.begin(), buffer.begin() + parsed_size, unparsed_size);
          }
        }
        // how to deal with error?
        return stdexec::just(check_parse_done(ec));
      }
    };

  } // namespace _parse_request

  inline constexpr _parse_request::parse_request_t parse_request{};

  namespace _recv_request {
    template <http_request Request>
    struct recv_state {
      Request request{};
      http1::RequestParser parser{&request};
      std::array<std::byte, 8192> buffer{};
      uint32_t unparsed_size{0};
      Request::duration remaining_time{0};
    };

    template <class Request>
    void initialize_state(recv_state<Request>& state) {
      // TODO(xiaoming): add concept limit
      constexpr auto opt = Request::socket_option();
      if (opt.keepalive_timeout != Request::unlimited_timeout) {
        state.remaining_time = opt.keepalive_timeout;
      } else {
        state.remaining_time = opt.total_timeout;
      }
    }

    template <class Request>
    void update_state(auto elapsed, std::size_t recv_size, recv_state<Request>& state) noexcept {
      state.unparsed_size += recv_size;
      (void) elapsed;
      // state.remaining_time -= elapsed.count(); // BUGBUG
    }

    // Get detailed error enum by message parser state.
    inline Error detailed_error(http1::RequestParser::MessageState state) noexcept {
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

    inline ex::sender auto check_recv_size(std::size_t recv_size) {
      return ex::if_then_else(
        recv_size != 0, //
        ex::just(recv_size),
        ex::just_error(http1::Error::kEndOfStream));
    }

    struct recv_request_t {
      template <http_request Request>
      stdexec::sender auto operator()(tcp_socket socket, Request& req) const noexcept {
        using timepoint = Request::timepoint;
        using recv_state = recv_state<Request>;
        auto scheduler = socket.context_->get_scheduler();

        return ex::just(recv_state{.request = req}) //
             | ex::let_value([&](recv_state& state) {
                 auto recv_buffer = [&] {
                   return std::span{state.buffer}.subspan(state.unparsed_size);
                 };

                 initialize_state(state);
                 return sio::async::read_some(socket, recv_buffer()) //
                      | ex::let_value(check_recv_size)               //
                      | ex::timeout(scheduler, state.remaining_time) //
                      | ex::let_stopped([&] {
                          return ex::just_error(detailed_error(state.parser.State()));
                        }) //
                      | ex::then([&](timepoint start, timepoint stop, std::size_t recv_size) {
                          state.request.update_metric(start, stop, recv_size);
                          update_state(stop - start, recv_size, state);
                        }) //
                      | ex::let_value([&] {
                          return parse_request(state.parser, state.buffer, state.unparsed_size);
                        })                        //
                      | ex::repeat_effect_until() //
                      | ex::then([&] { return std::move(state.request); });
               });
      }
    };
  } // namespace _recv_request

  inline constexpr _recv_request::recv_request_t recv_request{};

  namespace _handle_request {
    struct handle_request_t {
      template <class Request>
      ex::sender auto operator()(const Request& request) const noexcept {
        http1::Response response;
        response.status_code = http1::HttpStatusCode::kOK;
        response.version = request.version;
        return ex::just(response);
      }
    };

  } // namespace _handle_request

  inline constexpr _handle_request::handle_request_t handle_request{};

  namespace _create_response {
    struct create_response_t {
      ex::sender auto operator()(send_state& state) const {
        std::optional<std::string> response_str = state.response.MakeResponseString();
        if (response_str.has_value()) {
          state.start_line_and_headers = std::move(*response_str);
        }
        return ex::if_then_else(
          response_str.has_value(), ex::just(), ex::just_error(http1::Error::kInvalidResponse));
      }
    };
  } // namespace _create_response

  inline constexpr _create_response::create_response_t create_response{};

  namespace _start_server {
    template <http_request Request>
    bool check_keepalive(const Request& request) noexcept {
      if (request.ContainsHeader(http1::kHttpHeaderConnection)) {
        return true;
      }
      if (request.Version() == http1::HttpVersion::kHttp11) {
        return true;
      }
      return false;
    }

    session create_session(tcp_socket socket) {
      return session{socket};
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

    void update_session(session& session) noexcept {
    }

    struct start_server_t {
      void operator()(server& server) const noexcept {
        auto handles = sio::async::use_resources(
          [&](tcp_acceptor_handle acceptor) noexcept {
            return sio::async::accept(acceptor) //
                 | sio::let_value_each([&](tcp_socket socket) {
                     return ex::just(create_session(socket))                       //
                          | ex::let_value([&](session& session) {                  //
                              return recv_request(session.socket, session.request) //
                                   | ex::upon_error([](auto&&...) noexcept {});
                            });
                   })
                 | sio::ignore_all();
          },
          server.acceptor);
        ex::sync_wait(exec::when_any(handles, server.context.run()));
      }
    };
  } // namespace _start_server

  inline constexpr _start_server::start_server_t start_server{};

} // namespace net::http1

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
 */
