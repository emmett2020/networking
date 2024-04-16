/* Copyright 2023 Xiaoming Zhang
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
#include <catch2/catch_test_macros.hpp>

#include <span>
#include <system_error>
#include <utility>

#include "http/http_common.h"
#include "http/http_error.h"
#include "http/v1/http1_message_parser.h"
#include "http/v1/http1_request.h"

using namespace net::http;        // NOLINT
using namespace net::http::http1; // NOLINT
using namespace std;              // NOLINT

using request_parser = message_parser<http1_client_request>;
using request_line_state = request_parser::request_line_state;

TEST_CASE("parse http method", "[parse_http_request]") {
  http1_client_request req;
  request_parser parser{&req};

  SECTION("Parse valid GET method string should return http_method::get.") {
    string method = "GET ";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 3);
    CHECK(req.method == http_method::get);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::spaces_before_uri);
  }

  SECTION("Parse valid HEAD method string should return http_method::head.") {
    string method = "HEAD ";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 4);
    CHECK(req.method == http_method::head);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::spaces_before_uri);
  }

  SECTION("Parse valid POST method string should return http_method::post.") {
    string method = "POST ";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 4);
    CHECK(req.method == http_method::post);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::spaces_before_uri);
  }

  SECTION("Parse valid PUT method string should return http_method::put.") {
    string method = "PUT ";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 3);
    CHECK(req.method == http_method::put);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::spaces_before_uri);
  }

  SECTION("Parse valid DELETE method string should return http_method::del.") {
    string method = "DELETE ";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 6);
    CHECK(req.method == http_method::del);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::spaces_before_uri);
  }


  SECTION("Should return empty_method if there are whilespaces before method.") {
    // Treat all the characters before first space as method.
    // In this case, method is empty.
    string method = " GET ";
    auto result = parser.parse(std::span(method));
    REQUIRE(!result);
    CHECK(result.error() == error::empty_method);
  }

  SECTION("Should return unknown_method if method is not supported by networking.") {
    string method = "GXT ";
    auto result = parser.parse(std::span(method));
    REQUIRE(!result);
    CHECK(result.error() == error::unknown_method);
  }

  SECTION("Should return unknown_method if method isn't uppercase.") {
    string method = "GEt ";
    auto result = parser.parse(std::span(method));
    REQUIRE(!result);
    CHECK(result.error() == error::unknown_method);
  }

  SECTION("Should return bad_method if method is just a single non-token char.") {
    string method = "\"";
    auto result = parser.parse(std::span(method));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_method);
  }

  SECTION("Should return bad_method if method has not support token character.") {
    string method = "G(T ";
    auto result = parser.parse(std::span(method));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_method);
  }

  SECTION("Should return zero if parsing a method without whitespace followed by it.") {
    // Without whitespace we can't judge whether method read completed.
    string method = "GET";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 0);
    // State should convert from nothing_yet to start_line since there has a parse operation.
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::method);
  }

  SECTION(
    "Parse a method followed by multiply whitespaces should return the size of method which not "
    "include the size of whitespace") {
    // Three whitespaces after "CONNECT".
    string method = "CONNECT   ";
    auto result = parser.parse(std::span(method));
    REQUIRE(result);
    CHECK(*result == 7);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_parser::request_line_state::spaces_before_uri);
  }
}

TEST_CASE("parse http spaces between method and uri", "[parse_http_request]") {
  http1_client_request req;
  request_parser parser{&req};
  SECTION("Multiply whitespaces between method and uri are allowed.") {
    // Five spaces between method and URI.
    string buffer = "GET     /index.html ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);

    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }
}

TEST_CASE("parse http uri", "[parse_http_request]") {
  http1_client_request req;
  request_parser parser{&req};

  SECTION("Uri contains only path. Should fill the path field of request") {
    string buffer = "GET /index.html ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.path == "/index.html");
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }
  SECTION(
    "Uri only contains the http scheme is not allowed. Should return empty_host since http scheme "
    "must have host field.") {
    string buffer = "GET http:// ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::empty_host);
  }

  SECTION("Should return empty_host if https scheme doesn't have host field.") {
    // The "https" URI must have a non-empty host
    string buffer = "GET https://:80 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::empty_host);
  }

  SECTION("Uri contains uppercase scheme name is allowed.") {
    string buffer = "GET hTtP://domain ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.scheme == http_scheme::http);
    CHECK(req.host == "domain");
    CHECK(req.uri == "hTtP://domain");
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri contains https scheme and five whilespaces after the uri.") {
    string buffer = "GET         https://domain     ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 5);

    CHECK(req.scheme == http_scheme::https);
    CHECK(req.uri == "https://domain");
    CHECK(req.host == "domain");
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri contains unknown scheme name is allowed.") {
    string buffer = "GET 0unknown:// ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.uri == "0unknown://");
    CHECK(req.scheme == http_scheme::unknown);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Should return bad_scheme if scheme name contains invalid character.") {
    string buffer = "GET *:// ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_scheme);
  }

  SECTION("Should return bad_scheme if scheme name is invalid format.") {
    // The scheme format must be: "scheme-name" "://"
    // In this case, there are two colons after scheme-name "https".
    string buffer = "GET https::// ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_scheme);
  }

  SECTION("Uri contains scheme and host.") {
    string buffer = "GET https://domain ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "domain");
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri contains scheme and dot-decimal style host.") {
    string buffer = "GET https://1.1.1.1 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "1.1.1.1");
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Should return bad_host if host contains not supported character.") {
    string buffer = "GET https://doma*in ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_host);
  }

  SECTION(
    "Uri contains valid scheme, host and port should fill scheme, host and "
    "port field of inner request") {
    string buffer = "GET https://192.168.1.1:1080 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.uri == "https://192.168.1.1:1080");
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "192.168.1.1");
    CHECK(req.port == 1080);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION(
    "Uri contains scheme, host and port, port has leading zeros. Leading zeros should be "
    "ignored.") {
    string buffer = "GET https://domain:0001080 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.uri == "https://domain:0001080");
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "domain");
    CHECK(req.port == 1080);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION(
    "Uri contains scheme, host and port, port only have zeros. Should use default port based on "
    "scheme.") {
    string buffer = "GET https://domain:00000 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.uri == "https://domain:00000");
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "domain");
    CHECK(req.port == 443);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri doesn't contain port. Port should be 80 if scheme is http") {
    string buffer = "GET http://domain ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.scheme == http_scheme::http);
    CHECK(req.host == "domain");
    CHECK(req.port == 80);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri doesn't contain port. Port should be 443 if scheme is https") {
    string buffer = "GET https://domain ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "domain");
    CHECK(req.port == 443);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Should return bad_port if uri contains invalid port format.") {
    string buffer = "GET http://domain:8x0 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_port);
  }

  SECTION("Should return too_big_port if uri contains too big port.") {
    string buffer = "GET http://domain:65536 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::too_big_port);
  }

  SECTION("Uri contains scheme, host and path.") {
    string buffer = "GET http://domain/index.html ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "http://domain/index.html");
    CHECK(req.scheme == http_scheme::http);
    CHECK(req.host == "domain");
    CHECK(req.path == "/index.html");
    CHECK(req.port == 80);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri contains scheme, host, port and path.") {
    string buffer = "GET http://domain:10800/index.html ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "http://domain:10800/index.html");
    CHECK(req.scheme == http_scheme::http);
    CHECK(req.host == "domain");
    CHECK(req.path == "/index.html");
    CHECK(req.port == 10800);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Uri contains only path.") {
    // In this case, which scheme doesn't exist, host identifier is allowded to
    // be empty. And port should be 80.
    // TODO: should schemem default to be http or https if no scheme specified?
    string buffer = "GET /index.html ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "/index.html");
    CHECK(req.scheme == http_scheme::unknown);
    CHECK(req.host == "");
    CHECK(req.path == "/index.html");
    CHECK(req.port == 80);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }

  SECTION("Should return bad_path if uri contains non supported path character.") {
    string buffer = "GET /index.html\r123 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_path);
  }

  SECTION("Uri contains path and parameters.") {
    string buffer = "GET /index.html?key1=val1&key2=val2 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "/index.html?key1=val1&key2=val2");
    CHECK(req.scheme == http_scheme::unknown);
    CHECK(req.host == "");
    CHECK(req.port == 80);
    CHECK(req.path == "/index.html");
    CHECK(req.params.size() == 2);
    CHECK(req.params.contains("key1"));
    CHECK(req.params.contains("key2"));
    CHECK(req.params.find("key1")->second == "val1");
    CHECK(req.params.find("key2")->second == "val2");
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);
  }


  SECTION(
    "Uri contains path and parameters, the parameter has equal mark with "
    "empty value.") {
    string buffer = "GET /index.html?key= ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.params.size() == 1);
    CHECK(req.params.contains("key"));
    CHECK(req.params.find("key")->second == "");
    CHECK(parser.state_ == http1_parse_state::start_line);
  }

  SECTION("Uri contains path and parameters, the parameter has empty value.") {
    string buffer = "GET /index.html?key ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.params.size() == 1);
    CHECK(req.params.contains("key"));
    CHECK(req.params.find("key")->second == "");
    CHECK(parser.state_ == http1_parse_state::start_line);
  }

  SECTION("Uri contains path and parameters, the parameters only has adjacent ampersand.") {
    string buffer = "GET /index.html?&&&&&& ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.params.size() == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
  }

  // TODO: This case. Should we allow empty parameter name?
  SECTION("Uri contains path and parameters, the parameters only has adjacent equal mark") {
    string buffer = "GET /index.html?=== ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
  }

  // TODO: This case. Should we allow empty parameter name?
  SECTION("Uri contains path and parameters, the parameters only has ampersand and equal marks") {
    string buffer = "GET /index.html?&=&=&=&=& ";
  }

  SECTION("Uri contains path and parameters, the parameters has repeated key.") {
    string buffer = "GET /index.html?key=val0&key=val1 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.params.size() == 2);
    CHECK(req.params.count("key") == 2);
    auto range = req.params.equal_range("key");
    CHECK(range.first->second == "val0");
    CHECK((++range.first)->second == "val1");
    CHECK(parser.state_ == http1_parse_state::start_line);
  }

  SECTION("Should return bad_params if uri contains non supported parameter token.") {
    string buffer = "GET /index.html?key0\r=val0 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_params);
  }

  SECTION("Uri contains scheme, host, port, path and parameters.") {
    string buffer = "GET https://192.168.1.1:1080/index.html?key0=val0&key1=val1 ";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size() - 1);
    CHECK(req.scheme == http_scheme::https);
    CHECK(req.host == "192.168.1.1");
    CHECK(req.port == 1080);
    CHECK(req.path == "/index.html");
    CHECK(req.params.size() == 2);
    REQUIRE(req.params.contains("key0"));
    REQUIRE(req.params.contains("key1"));
    CHECK(req.params.find("key0")->second == "val0");
    CHECK(req.params.find("key1")->second == "val1");
    CHECK(parser.state_ == http1_parse_state::start_line);
  }

  SECTION("Step by step parse uri.") {
    string buffer = "PO";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::method);

    buffer += "ST ";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 4);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_uri);
    buffer.clear(); // Skip the parsed size in buffer.

    buffer += "htt";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::scheme);

    // http://dom
    buffer += "p://dom";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::host);

    // http://domain:10
    buffer += "ain:10";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::port);

    // http://domain:1080/index
    buffer += "80/index";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::path);

    // http://domain:1080/index.html?
    buffer += ".html?";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::params);

    // http://domain:1080/index.html?key
    buffer += "key";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::params);

    // http://domain:1080/index.html?key=
    buffer += "=";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::uri);
    CHECK(parser.uri_state_ == request_parser::uri_state::params);

    // http://domain:1080/index.html?key=val
    buffer += "val ";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 37);
    CHECK(parser.state_ == http1_parse_state::start_line);
    CHECK(parser.request_line_state_ == request_line_state::spaces_before_http_version);

    CHECK(req.uri == "http://domain:1080/index.html?key=val");
    CHECK(req.scheme == http_scheme::http);
    CHECK(req.host == "domain");
    CHECK(req.port == 1080);
    CHECK(req.path == "/index.html");
    CHECK(req.params.size() == 1);
    CHECK(req.params.contains("key"));
    CHECK(req.params.find("key")->second == "val");
  }
}

TEST_CASE("parse http version", "[parse_http_request]") {
  http1_client_request req;
  request_parser parser{&req};

  SECTION("Parse http version 1.0.") {
    string buffer = "GET /index.html HTTP/1.0\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "/index.html");
    CHECK(req.version == http_version::http10);
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
  }

  SECTION("Parse http version 1.1.") {
    string buffer = "GET /index.html HTTP/1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "/index.html");
    CHECK(req.version == http_version::http11);
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
  }

  SECTION("Multiply whitespaces between http version and uri are allowed.") {
    string buffer = "GET /index.html     HTTP/1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(req.method == http_method::get);
    CHECK(req.uri == "/index.html");
    CHECK(req.version == http_version::http11);
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
  }

  SECTION("Should return bad_version if http version is invalid.") {
    string buffer = "GET /index.html HTTP/1x1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_version);
  }

  SECTION("Should return bad_version if http version is not uppercase.") {
    string buffer = "GET /index.html http/1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_version);
  }
}

TEST_CASE("parse http headers", "[parse_http_request]") {
  http1_client_request req;
  request_parser parser{&req};

  SECTION("Parse http header Host.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host: 1.1.1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 1);
    REQUIRE(req.headers.contains("Host"));
    CHECK(req.headers.find("Host")->second == "1.1.1.1");
  }

  SECTION("Should return empty_header_name error when parse empty http header.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      ":1.1.1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::empty_header_name);
  }

  SECTION("Should return bad_header_name if http header contains with only non-token character.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "\r:Y\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_header_name);
  }

  SECTION("Should return bad_header_name if http header contains non-token character.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "H\r:Y\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::bad_header_name);
  }

  SECTION("Parse http header, header name only have one character.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "x:y\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 1);
    REQUIRE(req.headers.contains("x"));
    CHECK(req.headers.find("x")->second == "y");
  }

  SECTION("parse http header Host, multiply spaces before header value") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host:     1.1.1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 1);
    CHECK(req.headers.contains("Host"));
    CHECK(req.headers.find("Host")->second == "1.1.1.1");
  }

  SECTION("parse http header Host, multiply spaces after header value") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host: 1.1.1.1     \r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 1);
    CHECK(req.headers.contains("Host"));
    CHECK(req.headers.find("Host")->second == "1.1.1.1");
  }

  SECTION("parse http header Host, no spaces after header value") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host: 1.1.1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 1);
    CHECK(req.headers.contains("Host"));
    CHECK(req.headers.find("Host")->second == "1.1.1.1");
  }

  SECTION("parse http header Host, Host appear multiply times, save all repeated headers") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host: 1.1.1.1\r\n"
      "Host: 2.2.2.2\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 2);
    REQUIRE(req.headers.count("Host") == 2);
    auto range = req.headers.equal_range("Host");
    CHECK(range.first->second == "1.1.1.1");
    CHECK((++range.first)->second == "2.2.2.2");
  }

  SECTION(
    "parse http header Host, Host appear multiply times but with different case, save all repeated "
    "headers") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host: 1.1.1.1\r\n"
      "hoST: 2.2.2.2\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 2);
    REQUIRE(req.headers.count("Host") == 2);
    auto range = req.headers.equal_range("Host");
    CHECK(range.first->second == "1.1.1.1");
    CHECK((++range.first)->second == "2.2.2.2");
  }

  SECTION("Should return empty_header_value if http header value is empty.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host:\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(!result);
    CHECK(result.error() == error::empty_header_value);
  }

  SECTION("Parse multiply different http headers.") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Host:1.1.1.1\r\n"
      "Server:mock\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.size() == 2);
    CHECK(req.headers.contains("Host"));
    CHECK(req.headers.find("Host")->second == "1.1.1.1");
    CHECK(req.headers.contains("Server"));
    CHECK(req.headers.find("Server")->second == "mock");
  }

  SECTION("parse http header Accept") {
    // The Accept request HTTP header indicates which content types, expressed
    // as MIME types, the client is able to understand.
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Accept: image/*\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.contains("Accept"));
    CHECK(req.headers.find("Accept")->second == "image/*");
  }

  SECTION("parse http header Accept-Encoding which value is gzip") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Accept-Encoding: gzip\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.contains("Accept-Encoding"));
    CHECK(req.headers.find("Accept-Encoding")->second == "gzip");
  }

  SECTION("parse http header Connection") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Connection: Keep-Alive\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.contains("Connection"));
    CHECK(req.headers.find("Connection")->second == "Keep-Alive");
  }

  SECTION("parse http header Content-Encoding") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Content-Encoding: gzip\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.contains("Content-Encoding"));
    CHECK(req.headers.find("Content-Encoding")->second == "gzip");
  }

  SECTION("parse http header Content-Type") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Content-Type: text/html;charset=utf-8\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.contains("Content-Type"));
    CHECK(req.headers.find("Content-Type")->second == "text/html;charset=utf-8");
  }

  // SECTION("parse http header Content-Length, Content-Length has valid value") {
  //   string buffer =
  //     "GET /index.html HTTP/1.1\r\n"
  //     "Content-Length: 120\r\n";
  //   auto result = parser.parse(std::span(buffer));
  //   REQUIRE(result);
  //   CHECK(*result == buffer.size());
  //   CHECK(parser.state_ == http1_parse_state::expecting_newline);
  //   CHECK(req.headers.contains("Content-Length"));
  //   CHECK(req.headers.find("Content-Length")->second == "120");
  //   CHECK(req.content_length == 120);
  // }
  //
  // SECTION(
  //   "parse http header Content-Length, the length value is out of range "
  //   "should return bad_content_length") {
  //   string buffer =
  //     "GET /index.html HTTP/1.1\r\n"
  //     "Content-Length: 12000000000000000000000000000000\r\n";
  //   auto result = parser.parse(std::span(buffer));
  //   REQUIRE(!result);
  //   CHECK(result.error() == error::bad_content_length);
  // }
  //
  // SECTION(
  //   "parse http header Content-Length, the length string is invalid "
  //   "should return bad_content_length") {
  //   string buffer =
  //     "GET /index.html HTTP/1.1\r\n"
  //     "Content-Length: -10\r\n";
  //   auto result = parser.parse(std::span(buffer));
  //   REQUIRE(!result);
  //   CHECK(result.error() == error::bad_content_length);
  // }

  SECTION("parse http header Date") {
    string buffer =
      "GET /index.html HTTP/1.1\r\n"
      "Date: Thu, 11 Aug 2016 15:23:13 GMT\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
    CHECK(req.headers.contains("Date"));
    CHECK(req.headers.find("Date")->second == "Thu, 11 Aug 2016 15:23:13 GMT");
  }

  SECTION("Step by Step parse http header") {
    string buffer = "GET /index.html HTTP/1.1\r\n";
    auto result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(parser.state_ == http1_parse_state::expecting_newline);

    // Skip the parsed data.
    buffer = "";

    // Buffer: Ho
    buffer += "Ho";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::header);

    // Buffer: Host
    buffer += "st";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::header);

    // Buffer: Host:
    buffer += ":";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::header);

    // Buffer: Host:192.168.1.1
    buffer += "192.168.1.1";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::header);

    // Buffer: Host:192.168.1.1\r
    buffer += "\r";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == 0);
    CHECK(parser.state_ == http1_parse_state::header);

    // Buffer: Host:192.168.1.1\r\n
    buffer += "\n";
    result = parser.parse(std::span(buffer));
    REQUIRE(result);
    CHECK(*result == buffer.size());
    CHECK(req.headers.contains("Host"));
    CHECK(req.headers.find("Host")->second == "192.168.1.1");
    CHECK(parser.state_ == http1_parse_state::expecting_newline);
  }
}

//
// TEST_CASE("parse http request body") {
//   Request req;
//   request_parser parser(&req);
//   error_code ec{};
//
//   SECTION("No Content-Length header means no request body") {
//     string buffer =
//       "GET /index.html HTTP/1.1\r\n"
//       "\r\n";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == std::errc{});
//     CHECK(parser.state_ == http1_parse_state::kCompleted);
//     CHECK(req.body == "");
//   }
//
//   SECTION("The value of Content-Length is zero means no request body") {
//     string buffer =
//       "GET /index.html HTTP/1.1\r\n"
//       "Content-Length: 0\r\n"
//       "\r\n";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == std::errc{});
//     CHECK(parser.state_ == http1_parse_state::kCompleted);
//     CHECK(req.body == "");
//   }
//
//   SECTION("The value of Content-Length equals to request body") {
//     string buffer =
//       "GET /index.html HTTP/1.1\r\n"
//       "Content-Length: 11\r\n"
//       "\r\n"
//       "Hello World";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == std::errc{});
//     CHECK(parser.state_ == http1_parse_state::kCompleted);
//     CHECK(req.content_length == 11);
//     CHECK(req.body == "Hello World");
//   }
//
//   SECTION("Content-Length value less than body size") {
//     string buffer =
//       "GET /index.html HTTP/1.1\r\n"
//       "Content-Length: 1\r\n"
//       "\r\n"
//       "Hello World";
//     CHECK(parser.parse(buffer, ec) == 48);
//     CHECK(ec == std::errc{});
//     CHECK(parser.state_ == http1_parse_state::kCompleted);
//     CHECK(req.content_length == 1);
//     CHECK(req.body == "H");
//   }
// }
//
// TEST_CASE("parse http response status line") {
//   Response rsp;
//   Responseparser parser{&rsp};
//   std::error_code ec{};
//
//   SECTION("parse valid http1.1 version") {
//     std::string buffer = "HTTP/1.1 ";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == Error::kNeedMore);
//     CHECK(parser.state_ == Responseparser::MessageState::start_line);
//     CHECK(parser.status_line_state_ == Responseparser::StatusLineState::kStatusCode);
//     CHECK(rsp.version == http_version::http11);
//   }
//
//   SECTION("parse valid http1.0 version") {
//     std::string buffer = "HTTP/1.0 ";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == Error::kNeedMore);
//     CHECK(parser.state_ == Responseparser::MessageState::start_line);
//     CHECK(parser.status_line_state_ == Responseparser::StatusLineState::kStatusCode);
//     CHECK(rsp.version == http_version::http10);
//   }
//
//   SECTION("parse invalid http version") {
//     std::string buffer = "http/1.0 ";
//     CHECK(parser.parse(buffer, ec) == 0);
//     CHECK(ec == Error::kBadVersion);
//   }
//
//   SECTION("parse valid http status code") {
//     std::string buffer = "HTTP/1.1 200 ";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == Error::kNeedMore);
//     CHECK(parser.state_ == Responseparser::MessageState::start_line);
//     CHECK(parser.status_line_state_ == Responseparser::StatusLineState::kReason);
//     CHECK(rsp.version == http_version::http11);
//     CHECK(rsp.status_code == HttpStatusCode::kOK);
//   }
//
//   SECTION("parse invalid http status code, the code string length isn't 3") {
//     std::string buffer = "HTTP/1.1 2000 ";
//     CHECK(parser.parse(buffer, ec) == 0);
//     CHECK(ec == Error::kBadStatus);
//   }
//
//   SECTION("parse valid http reason") {
//     std::string buffer = "HTTP/1.1 200 OK\r\n";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == Error::kNeedMore);
//     CHECK(parser.state_ == Responseparser::MessageState::expecting_newline);
//     CHECK(rsp.version == http_version::http11);
//     CHECK(rsp.status_code == HttpStatusCode::kOK);
//     CHECK(rsp.reason == "OK");
//   }
//
//   SECTION("parse a complete http response") {
//     std::string buffer =
//       "HTTP/1.1 200 OK\r\n"
//       "Content-Length:11\r\n"
//       "\r\n"
//       "Hello world";
//     CHECK(parser.parse(buffer, ec) == buffer.size());
//     CHECK(ec == std::errc{});
//     CHECK(parser.state_ == Responseparser::MessageState::kCompleted);
//     CHECK(rsp.version == http_version::http11);
//     CHECK(rsp.status_code == HttpStatusCode::kOK);
//     CHECK(rsp.reason == "OK");
//     CHECK(rsp.content_length == 11);
//     CHECK(rsp.params.contains("Content-Length"));
//     CHECK(rsp.Body() == "Hello world");
//   }
// }
