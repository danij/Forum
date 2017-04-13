#pragma once

#pragma once

#include "HttpConstants.h"
#include "HttpStringHelpers.h"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace Http
{
    struct HttpRequest
    {
        //StringViews point to addresses inside the header buffer
        HttpVerb verb = HttpVerb::UNKNOWN;
        StringView path;
        int_fast8_t versionMajor = 1, versionMinor = 0;
        bool keepConnectionAlive = false;
        StringView headers[Request::HttpHeader::HTTP_HEADERS_COUNT];

        static constexpr size_t MaxQueryPairs = 64;
        std::pair<StringView, StringView> queryPairs[MaxQueryPairs];
        size_t nrOfQueryPairs = 0;
    };
}
