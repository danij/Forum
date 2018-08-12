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

#include "HttpParser.h"

#include <cassert>
#include <random>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace Http;
using namespace std::literals::string_literals;

static std::vector<size_t> getRandomSizes(size_t total)
{
    std::vector<size_t> result;

    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());

    while (total > 1)
    {
        std::uniform_int_distribution<size_t> randomDistribution(0u, total - 1);
        auto randomSize = randomDistribution(randomGenerator);
        assert(randomSize <= total);

        result.push_back(randomSize);
        total -= randomSize;
    }

    if (total > 0)
    {
        result.push_back(total);
    }

    return result;
}

template<typename TestCallback>
static void testParser(HttpStringView input, TestCallback&& callback, const size_t maxBodyLength = 1024, 
                       char* headerBuffer = nullptr, size_t headerBufferSize = 0)
{
    char buffer[1024];
    if (nullptr == headerBuffer)
    {
        headerBuffer = buffer;
        headerBufferSize = std::size(buffer);
    }

    std::string requestBody;

    Parser parser(headerBuffer, headerBufferSize, maxBodyLength, [](const char* buffer, size_t bufferSize, void* state)
    {
        reinterpret_cast<std::string*>(state)->append(buffer, bufferSize);
        return true;
    }, &requestBody);

    {
        std::string currentInput(input);

        parser.process(currentInput.data(), currentInput.size());
        callback(parser, requestBody);
    }
    parser.reset();
    requestBody.clear();
    {
        std::string currentInput(input);

        for (size_t i = 0; i < input.size(); ++i)
        {
            parser.process(currentInput.data() + i, size_t(1));
        }
        callback(parser, requestBody);
    }
    parser.reset();
    requestBody.clear();
    {
        std::string currentInput(input);
        auto randomSizes = getRandomSizes(input.size());
        auto currentPointer = currentInput.data();

        for (auto size : randomSizes)
        {
            parser.process(currentPointer, size);
            currentPointer += size;
        }
        callback(parser, requestBody);
    }
}

BOOST_AUTO_TEST_CASE( Http_Parser_result_is_empty_when_nothing_is_processed )
{
    testParser({}, [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(0, request.versionMinor);
        BOOST_REQUIRE_EQUAL(true, HttpVerb::UNKNOWN == request.verb);
        BOOST_REQUIRE_EQUAL(0u, request.path.size());
        BOOST_REQUIRE_EQUAL(false, request.keepConnectionAlive);
        BOOST_REQUIRE_EQUAL(decltype(request.remoteAddress){}, request.remoteAddress);
        for (const auto header : request.headers)
        {
            BOOST_REQUIRE_EQUAL(0u, header.size());
        }
        BOOST_REQUIRE_EQUAL(0u, request.nrOfQueryPairs);
        BOOST_REQUIRE_EQUAL(0u, request.nrOfCookies);
        BOOST_REQUIRE_EQUAL(0u, request.nrOfRequestContentBuffers);
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_supports_various_http_verbs )
{
    std::tuple<std::string, HttpVerb> toTest[] =
    {
        { "GET"s, HttpVerb::GET },
        { "POST"s, HttpVerb::POST },
        { "PUT"s, HttpVerb::PUT },
        { "PATCH"s, HttpVerb::PATCH },
        { "DELETE"s, HttpVerb::DELETE },
    };

    for (const auto& [verbString, expectedVerb] : toTest)
    {
        testParser(verbString + " / HTTP/1.0", [expected=expectedVerb](const Parser& parser, std::string_view /*requestBody*/)
        {
            BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
            BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

            const auto& request = parser.request();

            BOOST_REQUIRE_EQUAL(true, expected == request.verb);
        });
    }
}

BOOST_AUTO_TEST_CASE( Http_Parser_removes_trailing_slash_in_path )
{
    testParser("GET / HTTP/1.0", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("", request.path);
    });
    testParser("GET /hello/ HTTP/1.0", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("hello/", request.path);
    });
    testParser("GET ////test HTTP/1.0", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("test", request.path);
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_only_supports_version_10_and_11 )
{
    testParser("GET / HTTP/1.0\r\n", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(0, request.versionMinor);
    });
    testParser("GET / HTTP/1.1\r\n", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(1, request.versionMinor);
    });
    testParser("GET / HTTP/1.2\r\n", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::HTTP_Version_Not_Supported, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(0, request.versionMinor);
    });
    testParser("GET / HTTP/2.0\r\n", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::HTTP_Version_Not_Supported, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(0, request.versionMinor);
    });
    testParser("GET / HTTP/0\r\n", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::HTTP_Version_Not_Supported, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(0, request.versionMinor);
    });
    testParser("GET / HTTP/0.1.2\r\n", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::HTTP_Version_Not_Supported, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL(1, request.versionMajor);
        BOOST_REQUIRE_EQUAL(0, request.versionMinor);
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_decodes_url_encoding_in_path )
{
    testParser("GET /hello%20world/ HTTP/1.0", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("hello world/", request.path);
        BOOST_REQUIRE_EQUAL(0u, request.nrOfQueryPairs);
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_extracts_query_parameters )
{
    testParser("GET /app?bb=123&a=abcd%20e HTTP/1.0", [](const Parser& parser, std::string_view /*requestBody*/)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::ONGOING, parser);

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("app", request.path);

        BOOST_REQUIRE_EQUAL(2u, request.nrOfQueryPairs);
        BOOST_REQUIRE_EQUAL("bb", request.queryPairs[0].first);
        BOOST_REQUIRE_EQUAL("123", request.queryPairs[0].second);
        BOOST_REQUIRE_EQUAL("a", request.queryPairs[1].first);
        BOOST_REQUIRE_EQUAL("abcd e", request.queryPairs[1].second);
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_parses_only_known_headers )
{
    testParser("GET /app HTTP/1.0\r\nContent-leNGth:  10\r\nAbcd: abcde\r\nHOST: host_1\r\n\r\n", 
            [](const Parser& parser, std::string_view requestBody)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::FINISHED, parser);

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("app", request.path);

        for (size_t i = 0; i < static_cast<size_t>(Request::HttpHeader::HTTP_HEADERS_COUNT); ++i)
        {
            if (i == static_cast<size_t>(Request::HttpHeader::Content_Length))
            {
                BOOST_REQUIRE_EQUAL("10", request.headers[i]);
            }
            else if (i == static_cast<size_t>(Request::HttpHeader::Host))
            {
                BOOST_REQUIRE_EQUAL("host_1", request.headers[i]);
            }
            else
            {
                BOOST_REQUIRE_EQUAL("", request.headers[i]);
            }
        }

        BOOST_REQUIRE_EQUAL(0u, requestBody.size());
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_parses_cookies )
{
    testParser("GET /app HTTP/1.0\r\nCookie: a = 123;bb=4567; c=abcde%20f; just_value\r\n\r\n", 
            [](const Parser& parser, std::string_view requestBody)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::FINISHED, parser);

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("app", request.path);

        BOOST_REQUIRE_EQUAL(4u, request.nrOfCookies);
        BOOST_REQUIRE_EQUAL("a", request.cookies[0].first);
        BOOST_REQUIRE_EQUAL("123", request.cookies[0].second);
        BOOST_REQUIRE_EQUAL("bb", request.cookies[1].first);
        BOOST_REQUIRE_EQUAL("4567", request.cookies[1].second);
        BOOST_REQUIRE_EQUAL("c", request.cookies[2].first);
        BOOST_REQUIRE_EQUAL("abcde f", request.cookies[2].second);
        BOOST_REQUIRE_EQUAL("", request.cookies[3].first);
        BOOST_REQUIRE_EQUAL("just_value", request.cookies[3].second);

        BOOST_REQUIRE_EQUAL(0u, requestBody.size());
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_parses_request_body )
{
    testParser("POST /app HTTP/1.0\r\nContent-Length:11\r\n\r\naa\r\nbb%20cc", 
            [](const Parser& parser, std::string_view requestBody)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::FINISHED, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Bad_Request, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("app", request.path);

        BOOST_REQUIRE_EQUAL("aa\r\nbb%20cc", requestBody);
    });
}

BOOST_AUTO_TEST_CASE( Http_Parser_does_not_exceed_header_buffer )
{
    {
        char headerBuffer[5] = { 0 };
        const auto headerBufferSize = std::size(headerBuffer);

        testParser("GET /app HTTP/1.0\r\n\r\n",
               [&headerBuffer, headerBufferSize](const Parser& parser, std::string_view /*requestBody*/)
        {
            BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
            BOOST_REQUIRE_EQUAL(HttpStatusCode::Payload_Too_Large, parser.errorCode());

            const auto& request = parser.request();

            BOOST_REQUIRE_EQUAL("", request.path);
            BOOST_REQUIRE_EQUAL(0, headerBuffer[headerBufferSize - 1]);

        }, 1024, headerBuffer, headerBufferSize - 1);
    }
    {
        char headerBuffer[22] = { 0 };
        const auto headerBufferSize = std::size(headerBuffer);

        testParser("GET /app HTTP/1.0\r\nHost:host\r\n\r\n",
               [&headerBuffer, headerBufferSize](const Parser& parser, std::string_view /*requestBody*/)
        {
            BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
            BOOST_REQUIRE_EQUAL(HttpStatusCode::Payload_Too_Large, parser.errorCode());

            const auto& request = parser.request();

            BOOST_REQUIRE_EQUAL("app", request.path);
            BOOST_REQUIRE_EQUAL(0, headerBuffer[headerBufferSize - 1]);
            BOOST_REQUIRE_EQUAL("", request.headers[Request::HttpHeader::Host]);

        }, 1024, headerBuffer, headerBufferSize - 1);
    }
}

BOOST_AUTO_TEST_CASE( Http_Parser_does_not_exceed_content_buffer )
{
    testParser("POST /app HTTP/1.0\r\nContent-Length:11\r\n\r\naa\r\nbb%20cc",
        [](const Parser& parser, std::string_view requestBody)
    {
        BOOST_REQUIRE_EQUAL(Parser::ParseResult::INVALID_INPUT, parser);
        BOOST_REQUIRE_EQUAL(HttpStatusCode::Payload_Too_Large, parser.errorCode());

        const auto& request = parser.request();

        BOOST_REQUIRE_EQUAL("app", request.path);

        BOOST_REQUIRE_EQUAL("", requestBody);
    }, 10);
}
