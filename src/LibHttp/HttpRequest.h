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
        StringView path;
        int_fast8_t versionMajor = 1, versionMinor = 0;
        bool keepConnectionAlive = false;
        boost::asio::ip::address remoteAddress;
        StringView headers[Request::HttpHeader::HTTP_HEADERS_COUNT];

        static constexpr size_t MaxQueryPairs = 64;
        std::pair<StringView, StringView> queryPairs[MaxQueryPairs];
        size_t nrOfQueryPairs = 0;

        static constexpr size_t MaxCookies = 32;
        std::pair<StringView, StringView> cookies[MaxCookies];
        size_t nrOfCookies = 0;

        std::array<StringView, Buffer::MaximumBuffersForRequestBody> requestContentBuffers;
        size_t nrOfRequestContentBuffers = 0;
    };
}
