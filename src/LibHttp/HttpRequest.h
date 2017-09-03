#pragma once

#pragma once

#include "HttpConstants.h"
#include "HttpStringHelpers.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include <boost/asio/ip/address.hpp>

#ifdef DELETE
#undef DELETE
#endif

namespace Http
{
    struct HttpRequest
    {
        //StringViews point to addresses inside the header buffer
        HttpVerb verb = HttpVerb::UNKNOWN;
        HttpStringView path;
        int_fast8_t versionMajor = 1, versionMinor = 0;
        bool keepConnectionAlive = false;
        boost::asio::ip::address remoteAddress;
        HttpStringView headers[Request::HttpHeader::HTTP_HEADERS_COUNT];

        static constexpr size_t MaxQueryPairs = 64;
        std::pair<HttpStringView, HttpStringView> queryPairs[MaxQueryPairs];
        size_t nrOfQueryPairs = 0;

        static constexpr size_t MaxCookies = 32;
        std::pair<HttpStringView, HttpStringView> cookies[MaxCookies];
        size_t nrOfCookies = 0;

        std::array<HttpStringView, Buffer::MaximumBuffersForRequestBody> requestContentBuffers;
        size_t nrOfRequestContentBuffers = 0;
    };
}
