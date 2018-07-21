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

#include "HttpResponseBuilder.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

using namespace Http;

static const HttpStringView simpleResponseHeaders{"Connection: close\r\nContent-Length: 0\r\n"};

size_t Http::buildSimpleResponseFromStatusCode(const HttpStatusCode code, const int_fast8_t majorVersion, 
                                               const int_fast8_t minorVersion, char* buffer)
{
    assert(majorVersion >= 0);
    assert(majorVersion < 10);
    assert(minorVersion >= 0);
    assert(minorVersion < 10);

    const auto originalBufferStart = buffer;
    appendAndIncrement(buffer, "HTTP/");
    *buffer++ = majorVersion + '0';
    *buffer++ = '.';
    *buffer++ = minorVersion + '0';
    *buffer++ = ' ';

    const auto intCode = static_cast<int>(code);
    *buffer++ = (intCode / 100) + '0';
    *buffer++ = ((intCode / 10) % 10) + '0';
    *buffer++ = (intCode % 100) + '0';
    *buffer++ = ' ';

    auto codeString = getStatusCodeString(code);

    buffer = std::copy(codeString.begin(), codeString.end(), buffer);
    appendAndIncrement(buffer, "\r\n");

    buffer = std::copy(simpleResponseHeaders.begin(), simpleResponseHeaders.end(), buffer);
    appendAndIncrement(buffer, "\r\n");

    return buffer - originalBufferStart;
}

HttpResponseBuilder::HttpResponseBuilder(WriteFn writeFn, void* writeState)
    : writeFn_(writeFn), writeState_(writeState)
{
    assert(writeFn);
}

void HttpResponseBuilder::writeResponseCode(const int majorVersion, const int minorVersion, const HttpStatusCode code)
{
    assert(ProtocolState::NothingWritten == protocolState_);
    assert(1 == majorVersion);
    assert((0 == minorVersion) || (1 == minorVersion));

    char buffer[] = "HTTP/x.y zzz ";

    buffer[5] = majorVersion + '0';
    buffer[7] = minorVersion + '0';

    const auto intCode = static_cast<int>(code);
    buffer[9] = (intCode / 100) + '0';
    buffer[10] = ((intCode / 10) % 10) + '0';
    buffer[11] = (intCode % 100) + '0';

    write(buffer);
    write(getStatusCodeString(code));
    write("\r\n");

    protocolState_ = ProtocolState::ResponseCodeWritten;
}

void HttpResponseBuilder::writeResponseCode(const HttpRequest& request, const HttpStatusCode code)
{
    writeResponseCode(request.versionMajor, request.versionMinor, code);
}

void HttpResponseBuilder::writeHeader(const HttpStringView name, const HttpStringView value)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    write(name);
    write(": ");
    write(value);
    write("\r\n");
}

void HttpResponseBuilder::writeHeader(const HttpStringView name, const int value)
{
    char buffer[50];
    const auto written = sprintf(buffer, "%d", value);
    writeHeader(name, HttpStringView(buffer, written));
}

//based on information from http://www.ietf.org/rfc/rfc6265.txt
static const char ReservedCharactersForCookies[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static const char ReservedCharactersForUrlEncodingWithoutSlash[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

void HttpResponseBuilder::writeCookie(const HttpStringView name, const HttpStringView value, const CookieExtra extra)
{
    assert(name.length() < MaxPercentEncodingInputSize);
    assert(value.length() < MaxPercentEncodingInputSize);
    assert(extra.cookieDomain.length() < MaxPercentEncodingInputSize);
    assert(extra.cookiePath.length() < MaxPercentEncodingInputSize);

    assert(ProtocolState::ResponseCodeWritten == protocolState_);
    write("Set-Cookie: ");
    write(percentEncode(name, ReservedCharactersForCookies));
    write("=");
    write(percentEncode(value, ReservedCharactersForCookies));

    char extraBuffer[MaxPercentEncodingOutputSize * 2 + 256];
    char* buffer = extraBuffer;

    if (extra.expires)
    {
        appendAndIncrement(buffer, "; Expires=");
        buffer += writeHttpDateGMT(*extra.expires, buffer);
    }

    if (extra.cookieMaxAge)
    {
        appendAndIncrement(buffer, "; Max-Age=");
        buffer += sprintf(buffer, "%u", *extra.cookieMaxAge);
    }

    if (extra.cookieDomain.length())
    {
        appendAndIncrement(buffer, "; Domain=");

        auto encodedDomain = urlEncode(extra.cookieDomain);
        buffer = std::copy(encodedDomain.begin(), encodedDomain.end(), buffer);
    }

    if (extra.cookiePath.length())
    {
        appendAndIncrement(buffer, "; Path=");

        auto encodedPath = percentEncode(extra.cookiePath, ReservedCharactersForUrlEncodingWithoutSlash);
        buffer = std::copy(encodedPath.begin(), encodedPath.end(), buffer);
    }

    if (extra.isSecure)
    {
        appendAndIncrement(buffer, "; Secure");
    }

    if (extra.isHttpOnly)
    {
        appendAndIncrement(buffer, "; HttpOnly");
    }

    appendAndIncrement(buffer, "\r\n");

    write(extraBuffer, buffer - extraBuffer);
}

void HttpResponseBuilder::writeBody(const HttpStringView value)
{
    writeBody(value, {});
}

void HttpResponseBuilder::writeBody(const HttpStringView value, const HttpStringView prefix)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    write("\r\n");
    write(prefix.data(), prefix.size());
    write(value.data(), value.size());

    protocolState_ = ProtocolState::BodyWritten;
}

void HttpResponseBuilder::writeBodyAndContentLength(const HttpStringView value)
{
    writeBodyAndContentLength(value, {});
}

void HttpResponseBuilder::writeBodyAndContentLength(const HttpStringView value, const HttpStringView prefix)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    writeHeader("Content-Length", static_cast<int>(value.size() + prefix.size()));
    writeBody(value, prefix);
}
