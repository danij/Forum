/*
Fast Forum Backend
Copyright (C) Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "HttpResponseBuilder.h"

#include <string>

#include <boost/test/unit_test.hpp>

using namespace Http;
using namespace std::literals::string_literals;

BOOST_AUTO_TEST_CASE( BuildSimpleResponseFromStatusCode_works )
{
    char buffer[1024];
    const auto written = buildSimpleResponseFromStatusCode(HttpStatusCode::Not_Found, 1, 1, buffer);

    BOOST_REQUIRE_EQUAL("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n", 
                        HttpStringView(buffer, written));
}

BOOST_AUTO_TEST_CASE( WriteHttpDateGMT_works )
{
    char buffer[1024];
    const auto written = writeHttpDateGMT(1262401445, buffer);

    BOOST_REQUIRE_EQUAL("Sat, 02 Jan 2010 03:04:05 GMT", HttpStringView(buffer, written));
}

static void appendToString(const char* data, size_t size, void* state)
{
    reinterpret_cast<std::string*>(state)->append(data, size);
}

BOOST_AUTO_TEST_CASE( HttpReponseBuilder_writes_status_and_headers )
{
    std::string output;
    HttpResponseBuilder response{ appendToString, &output };

    response.writeResponseCode(1, 1, HttpStatusCode::No_Content);
    response.writeHeader("Header1", "Value1");
    response.writeHeader("Header2", 2000);
    response.writeHeader("Header3", "Value3");

    BOOST_REQUIRE_EQUAL("HTTP/1.1 204 No Content\r\nHeader1: Value1\r\nHeader2: 2000\r\nHeader3: Value3\r\n", output);
}

BOOST_AUTO_TEST_CASE( HttpReponseBuilder_writes_cookies )
{
    std::string output;
    HttpResponseBuilder response{ appendToString, &output };

    response.writeResponseCode(1, 1, HttpStatusCode::No_Content);
    response.writeHeader("Header1", "Value1");
    response.writeCookie("cookie1", "cookie value 1");
    response.writeCookie("cookie2", "cookie value 2", 
            CookieExtra{}.domain("domain2 ;").expiresAt(1444565594).httpOnly(true).path("/path; ").secure(true));
    response.writeHeader("Header3", "Value3");

    BOOST_REQUIRE_EQUAL(
        "HTTP/1.1 204 No Content\r\nHeader1: Value1\r\nSet-Cookie: cookie1=cookie%20value%201\r\nSet-Cookie: cookie2=cookie%20value%202; Expires=Sun, 11 Oct 2015 12:13:14 GMT; Domain=domain2%20%3B; Path=/path%3B%20; Secure; HttpOnly\r\nHeader3: Value3\r\n", 
        output);
}

BOOST_AUTO_TEST_CASE( HttpReponseBuilder_writes_body_without_content_length )
{
    std::string output;
    HttpResponseBuilder response{ appendToString, &output };

    response.writeResponseCode(1, 1, HttpStatusCode::OK);
    response.writeHeader("Header1", "Value1");
    response.writeBody("{\"a\": 1}", "while(1);");

    BOOST_REQUIRE_EQUAL("HTTP/1.1 200 OK\r\nHeader1: Value1\r\n\r\nwhile(1);{\"a\": 1}", output);
}

BOOST_AUTO_TEST_CASE( HttpReponseBuilder_writes_body_with_content_length )
{
    std::string output;
    HttpResponseBuilder response{ appendToString, &output };

    response.writeResponseCode(1, 1, HttpStatusCode::OK);
    response.writeHeader("Header1", "Value1");
    response.writeBodyAndContentLength("{\"a\": 1}", "while(1);");

    BOOST_REQUIRE_EQUAL("HTTP/1.1 200 OK\r\nHeader1: Value1\r\nContent-Length: 17\r\n\r\nwhile(1);{\"a\": 1}", output);
}
