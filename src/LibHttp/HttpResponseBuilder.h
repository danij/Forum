#pragma once

#include "HttpConstants.h"
#include "HttpRequest.h"

#include <cstddef>
#include <cstdint>
#include <ctime>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

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

    struct CookieExtra
    {
        CookieExtra& expiresAt(time_t value)
        {
            expires = value;
            return *this;
        }

        CookieExtra& maxAge(uint32_t seconds)
        {
            cookieMaxAge = seconds;
            return *this;
        }

        CookieExtra& domain(HttpStringView value)
        {
            cookieDomain = value;
            return *this;
        }

        CookieExtra& path(HttpStringView value)
        {
            cookiePath = value;
            return *this;
        }

        CookieExtra& secure(bool value)
        {
            isSecure = value;
            return *this;
        }

        CookieExtra& httpOnly(bool value)
        {
            isHttpOnly = value;
            return *this;
        }

        boost::optional<time_t> expires;
        boost::optional<uint32_t> cookieMaxAge;
        HttpStringView cookieDomain;
        HttpStringView cookiePath;
        bool isSecure = false;
        bool isHttpOnly = false;
    };

    class HttpResponseBuilder final : private boost::noncopyable
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
        void writeHeader(const char (&name)[NameSize], HttpStringView value)
        {
            writeHeader(HttpStringView(name, NameSize - 1), value);
        }

        template<size_t NameSize>
        void writeHeader(const char(&name)[NameSize], int value)
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
        void writeCookie(const char(&name)[NameSize], HttpStringView value, CookieExtra extra = {})
        {
            writeCookie(HttpStringView(name, NameSize - 1), value, std::move(extra));
        }

        void writeBody(const char* value, size_t length);
        void writeBodyAndContentLength(const char* value, size_t length);

    private:
        void write(const char* data, size_t size)
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
