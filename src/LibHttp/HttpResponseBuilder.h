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

#include "HttpConstants.h"
#include "HttpRequest.h"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <optional>

#include <boost/noncopyable.hpp>

namespace Http
{
    /**
     * Constructs a minimal HTTP response for the specified status code
     * @param code Status code which will be written both as a number and as a description
     * @param majorVersion Major version sent in the reply
     * @param minorVersion Minor version sent in the reply
     * @param buffer A large enough buffer (no bounds are checked)
     * @returns The number of bytes written
     */
    size_t buildSimpleResponseFromStatusCode(HttpStatusCode code, int_fast8_t majorVersion, int_fast8_t minorVersion,
                                             char* buffer);
    
    /**
    * Writes a date string as expected by the HTTP protocol, e.g. Tue, 18 Apr 2017 09:00:00 GMT
    * The functions doesn't check bounds, the output buffer must be large enough
    *
    * @return The amount of characters written
    */
    size_t writeHttpDateGMT(time_t value, char* output);

    struct CookieExtra final
    {
        CookieExtra& expiresAt(const time_t value)
        {
            expires = value;
            return *this;
        }

        CookieExtra& maxAge(const uint32_t seconds)
        {
            cookieMaxAge = seconds;
            return *this;
        }

        CookieExtra& domain(const HttpStringView value)
        {
            cookieDomain = value;
            return *this;
        }

        CookieExtra& path(const HttpStringView value)
        {
            cookiePath = value;
            return *this;
        }

        CookieExtra& secure(const bool value)
        {
            isSecure = value;
            return *this;
        }

        CookieExtra& httpOnly(const bool value)
        {
            isHttpOnly = value;
            return *this;
        }

        std::optional<time_t> expires;
        std::optional<uint32_t> cookieMaxAge;
        HttpStringView cookieDomain;
        HttpStringView cookiePath;
        bool isSecure = false;
        bool isHttpOnly = false;
    };

    class HttpResponseBuilder final : boost::noncopyable
    {
    public:
        typedef void (*WriteFn)(const char* data, size_t size, void* state);

        HttpResponseBuilder(WriteFn writeFn, void* writeState);

        void reset()
        {
            protocolState_ = ProtocolState::NothingWritten;
        }

        void writeResponseCode(int majorVersion, int minorVersion, HttpStatusCode code);
        void writeResponseCode(const HttpRequest& request, HttpStatusCode code);
        void writeHeader(HttpStringView name, HttpStringView value);
        void writeHeader(HttpStringView name, int value);

        template<size_t NameSize>
        void writeHeader(const char (&name)[NameSize], const HttpStringView value)
        {
            writeHeader(HttpStringView(name, NameSize - 1), value);
        }

        template<size_t NameSize>
        void writeHeader(const char(&name)[NameSize], const int value)
        {
            writeHeader(HttpStringView(name, NameSize - 1), value);
        }

        template<size_t NameSize, size_t ValueSize>
        void writeHeader(const char(&name)[NameSize], const char (&value)[ValueSize])
        {
            writeHeader(HttpStringView(name, NameSize - 1), HttpStringView(value, ValueSize - 1));
        }

        void writeCookie(HttpStringView name, HttpStringView value, CookieExtra extra = {});

        template<size_t NameSize>
        void writeCookie(const char(&name)[NameSize], const HttpStringView value, CookieExtra extra = {})
        {
            writeCookie(HttpStringView(name, NameSize - 1), value, std::move(extra));
        }

        void writeBody(HttpStringView value);
        void writeBody(HttpStringView value, HttpStringView prefix);
        void writeBodyAndContentLength(HttpStringView value);
        void writeBodyAndContentLength(HttpStringView value, HttpStringView prefix);

    private:
        void write(const char* data, const size_t size)
        {
            writeFn_(data, size, writeState_);
        }

        void write(HttpStringView view)
        {
            write(view.data(), view.size());
        }

        template<size_t Size>
        void write(const char (&data)[Size])
        {
            write(data, Size - 1);
        }

        enum class ProtocolState
        {
            NothingWritten,
            ResponseCodeWritten,
            BodyWritten,
        };

        ProtocolState protocolState_ = ProtocolState::NothingWritten;
        WriteFn writeFn_;
        void* writeState_;
    };
}
