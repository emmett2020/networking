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

#include <string>
#include <unordered_map>
#include <vector>

#include <sio/ip/endpoint.hpp>
#include "http/http_common.h"

namespace net::http {

// A request received from a client.
struct Request {
  HttpMethod method = HttpMethod::kHttpMethodUnknown;
  std::string host;
  sio::ip::port_type port;
  std::string url;
  std::string path;
  uint8_t http_version_major = 0;
  uint8_t http_version_minor = 0;
  std::unordered_map<std::string, std::string> headers;
  std::unordered_map<std::string, std::string> params;
};

}  // namespace net::http
