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
#include "http1/http_common.h"
#include "http1/http_error.h"
#include "http1/http_message_parser.h"
#include "http1/http_request.h"

using namespace net::http1;  // NOLINT
using namespace std;         // NOLINT

using RequestParser = MessageParser<Request>;

TEST_CASE("Parse http method", "[parse_http_request]") {
  Request req;
  RequestParser parser{&req};
  std::error_code ec{};

  SECTION("Parse valid GET method string should return HttpMethod::kGet") {
    string method = "GET ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION("Parse valid HEAD method string should return HttpMethod::kHead") {
    string method = "HEAD ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kHead);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION("Parse valid POST method string should return HttpMethod::kPost") {
    string method = "POST ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kPost);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION("Parse valid PUT method string should return HttpMethod::kPut") {
    string method = "PUT ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kPut);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION(
      "Parse valid DELETE method string should return HttpMethod::kDelete") {
    string method = "DELETE ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kDelete);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION("Parse valid TRACE method string should return HttpMethod::kTrace") {
    string method = "TRACE ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kTrace);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION(
      "Parse valid CONTROL method string should return HttpMethod::kControl") {
    string method = "CONTROL ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kControl);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION("Parse valid PURGE method string should return HttpMethod::kPurge") {
    string method = "PURGE ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kPurge);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION(
      "Parse valid OPTIONS method string should return HttpMethod::kOptions") {
    string method = "OPTIONS ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kOptions);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION(
      "Parse valid CONNECT method string should return HttpMethod::kConnect") {
    string method = "CONNECT ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) ==
          method.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kConnect);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }

  SECTION(
      "Parse invalid method string, whitespace before method should return "
      "kEmptyMethod") {
    // Treat all the characters before first space as method.
    // In this case, method is empty not "GET".
    string method = " GET ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kEmptyMethod);
  }

  SECTION("Parse invalid method string, method isn't uppercase") {
    string method = "GEt ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kBadMethod);
  }

  SECTION("Parse invalid method string, not supported method string") {
    string method = "GXT ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kBadMethod);
  }

  SECTION("Parse invalid method string, method is a single non-token char") {
    string method = "\"";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kBadMethod);
  }

  SECTION("Parse invalid method string, valid token but not supported method") {
    string method = "G*T ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kBadMethod);
  }

  SECTION("Parse invalid method string, method has not support token char") {
    string method = "G(T ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kBadMethod);
  }

  SECTION(
      "Parse a method without whitespace followed by it should return "
      "kNeedMore") {
    // Because without whitespace we can't judge whether method read completed.
    string method = "GET";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
  }

  SECTION(
      "Parse multiply whitespace followed method should return real method "
      "size which not include the size of whitespace") {
    // Three whitespaces after "CONNECT".
    string method = "CONNECT   ";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 7);
    CHECK(ec == Error::kNeedMore);
  }

  SECTION("Step by step parsing method should work") {
    string method = "P";
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kMethod);

    ec.clear();
    method.push_back('O');
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kMethod);

    ec.clear();
    method.push_back('S');
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kMethod);

    ec.clear();
    method.push_back('T');
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kMethod);

    ec.clear();
    method.push_back(' ');
    CHECK(parser.Parse({method.data(), method.size()}, ec) == 4);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);
  }
}

TEST_CASE("Parse http uri", "[parse_http_request]") {
  Request req;
  RequestParser parser(&req);
  error_code ec{};

  SECTION("Uri contains only path. Should fill the path field of request") {
    string buffer = "GET /index.html ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Path() == "/index.html");

    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Multi spaces between method and uri is allowed") {
    // Five spaces between method and URI.
    string buffer = "GET     /index.html ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Uri() == "/index.html");
    CHECK(req.Path() == "/index.html");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri only contains the http scheme is not allowed.") {
    // The "http" scheme must have host field.
    string buffer = "GET http:// ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadHost);
  }

  SECTION("A https uri contains empty host is not allowed.") {
    // The "https" URI must have a non-empty host
    string buffer = "GET https://:80 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadHost);
  }

  SECTION("Uri contains scheme name in uppercase is allowed") {
    string buffer = "GET hTtP://domain ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.scheme == HttpScheme::kHttp);
    CHECK(req.Host() == "domain");
    CHECK(req.Uri() == "hTtP://domain");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains https scheme and five spaces after the uri") {
    string buffer = "GET         https://domain     ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 5);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.scheme == HttpScheme::kHttps);
    CHECK(req.Uri() == "https://domain");
    CHECK(req.Host() == "domain");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains unknown scheme name") {
    string buffer = "GET 0unknown:// ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Uri() == "0unknown://");
    CHECK(req.scheme == HttpScheme::kUnknown);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains invalid scheme, scheme name use invalid character") {
    string buffer = "GET *:// ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadScheme);
  }

  SECTION("Uri contains invalid scheme, scheme use invalid format") {
    // The scheme format must be: "scheme-name" "://"
    // But there are two colon after scheme-name "https".
    string buffer = "GET https::// ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadScheme);
  }

  SECTION("Uri contains scheme and host domain style host identifier") {
    string buffer = "GET https://domain ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "domain");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains scheme and dot-decimal style host identifier") {
    string buffer = "GET https://1.1.1.1 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "1.1.1.1");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Host has not supported character should return kBadHost") {
    string buffer = "GET https://doma*in ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadHost);
  }

  SECTION(
      "Uri contains valid scheme, host and port should fill scheme, host and "
      "port field of inner request") {
    string buffer = "GET https://192.168.1.1:1080 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Uri() == "https://192.168.1.1:1080");
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "192.168.1.1");
    CHECK(req.Port() == 1080);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains scheme, host and port, port has leading zeros.") {
    string buffer = "GET https://domain:0001080 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Uri() == "https://domain:0001080");
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "domain");
    CHECK(req.Port() == 1080);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION(
      "Uri contains scheme, host and port, port only have zeros should use "
      "default port value based on scheme.") {
    string buffer = "GET https://domain:00000 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Uri() == "https://domain:00000");
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "domain");
    CHECK(req.Port() == 443);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION(
      "Uri doesn't contain port identifier. Port will default be 80 if "
      "scheme is http") {
    string buffer = "GET http://domain ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Scheme() == HttpScheme::kHttp);
    CHECK(req.Host() == "domain");
    CHECK(req.Port() == 80);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION(
      "Uri doesn't contain port identifier. Port will default to be 443 if "
      "scheme is https") {
    string buffer = "GET https://domain ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "domain");
    CHECK(req.Port() == 443);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains invalid port format should return bad port.") {
    string buffer = "GET http://domain:8x0 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadPort);
  }

  SECTION("Uri contains too big port should return bad port.") {
    string buffer = "GET http://domain:65536 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadPort);
  }

  SECTION("Uri contains too big port should return bad port.") {
    string buffer = "GET http://domain:65536 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadPort);
  }

  SECTION("Uri contains scheme, host and path") {
    string buffer = "GET http://domain/index.html ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(req.Uri() == "http://domain/index.html");
    CHECK(req.Scheme() == HttpScheme::kHttp);
    CHECK(req.Host() == "domain");
    CHECK(req.Path() == "/index.html");
    CHECK(req.Port() == 80);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains scheme, host, port and path") {
    string buffer = "GET http://domain:10800/index.html ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(req.Uri() == "http://domain:10800/index.html");
    CHECK(req.Scheme() == HttpScheme::kHttp);
    CHECK(req.Host() == "domain");
    CHECK(req.Path() == "/index.html");
    CHECK(req.Port() == 10800);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains only path") {
    // In this case, which scheme doesn't exist, host identifier is allowded to
    // be empty. And port should be 80.
    string buffer = "GET /index.html ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html");
    CHECK(req.Scheme() == HttpScheme::kUnknown);
    CHECK(req.Host() == "");
    CHECK(req.Path() == "/index.html");
    CHECK(req.Port() == 80);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains non supported path character should return kBadPath") {
    string buffer = "GET /index.html\r123 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadPath);
  }

  SECTION("Uri contains path and parameters") {
    string buffer = "GET /index.html?key1=val1&key2=val2 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html?key1=val1&key2=val2");
    CHECK(req.Scheme() == HttpScheme::kUnknown);
    CHECK(req.Host() == "");
    CHECK(req.Port() == 80);
    CHECK(req.Path() == "/index.html");
    CHECK(req.Params().size() == 2);
    CHECK(req.ContainsParam("key1"));
    CHECK(req.ContainsParam("key2"));
    CHECK(req.ParamValue("key1").value() == "val1");
    CHECK(req.ParamValue("key2").value() == "val2");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Uri contains path and parameters, the parameter has empty key") {
    string buffer = "GET /index.html?=val ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html?=val");
    CHECK(req.Scheme() == HttpScheme::kUnknown);
    CHECK(req.Host() == "");
    CHECK(req.Port() == 80);
    CHECK(req.Path() == "/index.html");
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam(""));
    CHECK(req.ParamValue("").value() == "val");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION(
      "Uri contains path and parameters, the parameter has equal mark with "
      "empty value") {
    string buffer = "GET /index.html?key= ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam("key"));
    CHECK(req.ParamValue("key").value() == "");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION("Uri contains path and parameters, the parameter has empty value") {
    string buffer = "GET /index.html?key ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam("key"));
    CHECK(req.ParamValue("key").value() == "");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION(
      "Uri contains path and parameters, the parameters only has adjacent "
      "ampersand") {
    string buffer = "GET /index.html?&&&&&& ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Params().size() == 0);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION(
      "Uri contains path and parameters, the parameters only has adjacent "
      "equal mark") {
    string buffer = "GET /index.html?=== ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam(""));
    CHECK(req.ParamValue("") == "==");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION(
      "Uri contains path and parameters, the parameters only has ampersand and "
      "equal marks") {
    string buffer = "GET /index.html?&=&=&=&=& ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam(""));
    CHECK(req.ParamValue("") == "");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION("Uri contains path and parameters, the parameters has repeated key") {
    string buffer = "GET /index.html?key=val0&key=val1 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam("key"));
    CHECK(req.ParamValue("key") == "val1");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION("Uri contains non supported parameter token should kBadParameter") {
    string buffer = "GET /index.html?key0\r=val0 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadParams);
  }

  SECTION("Uri contains scheme, host, port, path and parameters") {
    string buffer =
        "GET https://192.168.1.1:1080/index.html?key0=val0&key1=val1 ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Scheme() == HttpScheme::kHttps);
    CHECK(req.Host() == "192.168.1.1");
    CHECK(req.Port() == 1080);
    CHECK(req.Path() == "/index.html");
    CHECK(req.Params().size() == 2);
    CHECK(req.ContainsParam("key0"));
    CHECK(req.ContainsParam("key1"));
    CHECK(req.ParamValue("key0") == "val0");
    CHECK(req.ParamValue("key1") == "val1");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
  }

  SECTION("Uri contains path and a parameter") {
    string buffer = "GET /index.html?key=val ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) ==
          buffer.size() - 1);
    CHECK(ec == Error::kNeedMore);
    CHECK(req.method == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html?key=val");
    CHECK(req.Scheme() == HttpScheme::kUnknown);
    CHECK(req.Host() == "");
    CHECK(req.Port() == 80);
    CHECK(req.Path() == "/index.html");
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam("key"));
    CHECK(req.ParamValue("key").value() == "val");
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
  }

  SECTION("Step by step parse uri") {
    string buffer = "PO";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kMethod);

    ec.clear();
    buffer += "ST ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 4);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeUri);

    ec.clear();
    // Skip the parsed size in buffer.
    buffer.clear();
    buffer += "htt";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kScheme);
    CHECK(parser.inner_parsed_len_ == 0);

    ec.clear();
    // http://dom
    buffer += "p://dom";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kHost);
    CHECK(parser.inner_parsed_len_ == 7);

    ec.clear();
    // http://domain:10
    buffer += "ain:10";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kPort);
    // The "http://domain:" is done.
    CHECK(parser.inner_parsed_len_ == 7 + 7);

    ec.clear();
    // http://domain:1080/index
    buffer += "80/index";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kPath);
    // The "http://domain:1080" is done.
    CHECK(parser.inner_parsed_len_ == 7 + 7 + 4);

    ec.clear();
    // http://domain:1080/index.html?
    buffer += ".html?";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kParams);
    // The "http://domain:1080/index.html?" is done.
    CHECK(parser.inner_parsed_len_ == 7 + 7 + 4 + 12);

    ec.clear();
    // http://domain:1080/index.html?key
    buffer += "key";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kParams);
    // The "http://domain:1080/index.html?" is done.
    CHECK(parser.inner_parsed_len_ == 7 + 7 + 4 + 12);

    ec.clear();
    // http://domain:1080/index.html?key=
    buffer += "=";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ == RequestParser::RequestLineState::kUri);
    CHECK(parser.uri_state_ == RequestParser::UriState::kParams);
    // The "http://domain:1080/index.html?" is done.
    CHECK(parser.inner_parsed_len_ == 7 + 7 + 4 + 12 + 4);

    ec.clear();
    // http://domain:1080/index.html?key=val
    buffer += "val ";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 37);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kStartLine);
    CHECK(parser.request_line_state_ ==
          RequestParser::RequestLineState::kSpacesBeforeHttpVersion);
    // The "http://domain:1080/index.html?key=val " is done.
    CHECK(parser.inner_parsed_len_ == 0);

    CHECK(req.Uri() == "http://domain:1080/index.html?key=val");
    CHECK(req.scheme == HttpScheme::kHttp);
    CHECK(req.Host() == "domain");
    CHECK(req.Port() == 1080);
    CHECK(req.Path() == "/index.html");
    CHECK(req.Params().size() == 1);
    CHECK(req.ContainsParam("key"));
    CHECK(req.ParamValue("key") == "val");
  }
}

TEST_CASE("Parse http version", "[parse_http_request]") {
  Request req;
  RequestParser parser(&req);
  error_code ec{};

  SECTION("Parse http version 1.0") {
    string buffer = "GET /index.html HTTP/1.0\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Method() == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html");
    CHECK(req.Path() == "/index.html");
    CHECK(req.Version() == HttpVersion::kHttp10);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
  }

  SECTION("Parse http version 1.1") {
    string buffer = "GET /index.html HTTP/1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Method() == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html");
    CHECK(req.Path() == "/index.html");
    CHECK(req.Version() == HttpVersion::kHttp11);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
  }

  SECTION("Multiply spaces between http version and uri are allowed") {
    string buffer = "GET /index.html     HTTP/1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(req.Method() == HttpMethod::kGet);
    CHECK(req.Uri() == "/index.html");
    CHECK(req.Path() == "/index.html");
    CHECK(req.Version() == HttpVersion::kHttp11);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
  }

  SECTION("Parse invalid http version should return kBadVersion") {
    string buffer = "GET /index.html HTTP/1x1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadVersion);
  }

  SECTION("http version should be all uppercase") {
    string buffer = "GET /index.html http/1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadVersion);
  }
}

TEST_CASE("Parse http headers", "[parse_http_request]") {
  Request req;
  RequestParser parser(&req);
  error_code ec{};

  SECTION("Parse http header Host") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host: 1.1.1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "1.1.1.1");
  }

  SECTION("Parse empty http header should return kEmptyHeaderName") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        ":1.1.1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kEmptyHeaderName);
  }

  SECTION(
      "Parse http header with only non-token character should return "
      "kBadHeaderName") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "\r:Y\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadHeaderName);
  }

  SECTION(
      "Parse http header with non-token character should return "
      "kBadHeaderName") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "H\r:Y\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadHeaderName);
  }

  SECTION("Parse http header, the header name only have one character") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "x:y\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("x"));
    CHECK(req.HeaderValue("x") == "y");
  }

  SECTION("Parse http header Host, multiply spaces before header value") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host:     1.1.1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "1.1.1.1");
  }

  SECTION("Parse http header Host, multiply spaces after header value") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host: 1.1.1.1     \r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "1.1.1.1");
  }

  SECTION("Parse http header Host, no spaces after header value") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host: 1.1.1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "1.1.1.1");
  }

  SECTION(
      "Parse http header Host, Host appear multiply times, use the last "
      "appeared value") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host: 1.1.1.1\r\n"
        "Host: 2.2.2.2\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "2.2.2.2");
  }

  SECTION(
      "Parse http header Host, Host appears multiply times but with different "
      "character case, use the last appeared value") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host: 1.1.1.1\r\n"
        "hoST: 2.2.2.2\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 1);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "2.2.2.2");
  }

  SECTION("Parse empty http header value should return kEmptyHeaderValue") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host:\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kEmptyHeaderValue);
  }

  SECTION("Parse multiply http header") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Host:1.1.1.1\r\n"
        "Server:mock\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.Headers().size() == 2);
    CHECK(req.ContainsHeader("Host"));
    CHECK(req.HeaderValue("Host") == "1.1.1.1");
    CHECK(req.ContainsHeader("Server"));
    CHECK(req.HeaderValue("Server") == "mock");
  }

  SECTION("Parse http header Accept") {
    // The Accept request HTTP header indicates which content types, expressed
    // as MIME types, the client is able to understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept: image/*\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept"));
    CHECK(req.HeaderValue("Accept") == "image/*");
  }

  SECTION("Parse http header Accept-Encoding which value is gzip") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding: gzip\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "gzip");
  }

  SECTION("Parse http header Accept-Encoding which value is compress") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding: compress\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "compress");
  }

  SECTION("Parse http header Accept-Encoding which value is deflate") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding: deflate\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "deflate");
  }

  SECTION("Parse http header Accept-Encoding which value is br") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding: br\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "br");
  }

  SECTION("Parse http header Accept-Encoding which value is identity") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding: identity\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "identity");
  }

  SECTION("Parse http header Accept-Encoding which value is *") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding: *\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "*");
  }
  SECTION(

      "Parse http header Accept-Encoding, header occurs multiply times, "
      "combine all values in order") {
    // The Accept-Encoding request HTTP header indicates the content encoding
    // (usually a compression algorithm) that the client can understand.
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Accept-Encoding:*\r\n"
        "Accept-Encoding:gzip\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Accept-Encoding"));
    CHECK(req.HeaderValue("Accept-Encoding") == "*,gzip");
  }

  SECTION("Parse http header Connection") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Connection: Keep-Alive\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Connection"));
    CHECK(req.HeaderValue("Connection") == "Keep-Alive");
  }

  SECTION("Parse http header Content-Encoding") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Encoding: gzip\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Content-Encoding"));
    CHECK(req.HeaderValue("Content-Encoding") == "gzip");
  }

  SECTION("Parse http header Content-Type") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Type: text/html;charset=utf-8\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Content-Type"));
    CHECK(req.HeaderValue("Content-Type") == "text/html;charset=utf-8");
  }

  SECTION("Parse http header Content-Length, Content-Length has valid value") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Length: 120\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Content-Length"));
    CHECK(req.HeaderValue("Content-Length") == "120");
    CHECK(req.content_length == 120);
  }

  SECTION(
      "Parse http header Content-Length, the length value is out of range "
      "should return kBadContentLength") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Length: 12000000000000000000000000000000\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadContentLength);
  }

  SECTION(
      "Parse http header Content-Length, the length string is invalid "
      "should return kBadContentLength") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Length: -10\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kBadContentLength);
  }

  SECTION("Parse http header Date") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Date: Thu, 11 Aug 2016 15:23:13 GMT\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
    CHECK(req.ContainsHeader("Date"));
    CHECK(req.HeaderValue("Date") == "Thu, 11 Aug 2016 15:23:13 GMT");
  }

  SECTION("Step by Step parse http header") {
    string buffer = "GET /index.html HTTP/1.1\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);

    // Skip the parsed data.
    buffer = "";

    // Buffer: Ho
    buffer += "Ho";
    ec.clear();
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kHeader);

    // Buffer: Host
    buffer += "st";
    ec.clear();
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kHeader);

    // Buffer: Host:
    buffer += ":";
    ec.clear();
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kHeader);

    // Buffer: Host:192.168.1.1
    buffer += "192.168.1.1";
    ec.clear();
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kHeader);

    // Buffer: Host:192.168.1.1\r
    buffer += "\r";
    ec.clear();
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 0);
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ == RequestParser::MessageState::kHeader);

    // Buffer: Host:192.168.1.1\r\n
    buffer += "\n";
    ec.clear();
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == Error::kNeedMore);
    CHECK(parser.message_state_ ==
          RequestParser::MessageState::kExpectingNewline);
  }
}

TEST_CASE("parse http request body") {
  Request req;
  RequestParser parser(&req);
  error_code ec{};

  SECTION("No Content-Length header means no request body") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == std::errc{});
    CHECK(parser.message_state_ == RequestParser::MessageState::kCompleted);
    CHECK(req.body == "");
  }

  SECTION("The value of Content-Length is zero means no request body") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == std::errc{});
    CHECK(parser.message_state_ == RequestParser::MessageState::kCompleted);
    CHECK(req.body == "");
  }

  SECTION("The value of Content-Length equals to request body") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == buffer.size());
    CHECK(ec == std::errc{});
    CHECK(parser.message_state_ == RequestParser::MessageState::kCompleted);
    CHECK(req.content_length == 11);
    CHECK(req.body == "Hello World");
  }

  SECTION("Content-Length value less than body size") {
    string buffer =
        "GET /index.html HTTP/1.1\r\n"
        "Content-Length: 1\r\n"
        "\r\n"
        "Hello World";
    CHECK(parser.Parse({buffer.data(), buffer.size()}, ec) == 48);
    CHECK(ec == std::errc{});
    CHECK(parser.message_state_ == RequestParser::MessageState::kCompleted);
    CHECK(req.content_length == 1);
    CHECK(req.body == "H");
  }
}
