/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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
    struct HttpRequest final
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

        HttpStringView getCookie(const HttpStringView searchName) const
        {
            for (size_t i = 0; i < nrOfCookies; ++i)
            {
                const auto&[name, value] = cookies[i];
                if (name == searchName)
                {
                    return value;
                }
            }
            return {};
        }
    };
}
