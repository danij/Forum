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

size_t Http::buildSimpleResponseFromStatusCode(HttpStatusCode code, int_fast8_t majorVersion, int_fast8_t minorVersion,
                                               char* buffer)
{
    auto originalBufferStart = buffer;
    *buffer++ = 'H';
    *buffer++ = 'T';
    *buffer++ = 'T';
    *buffer++ = 'P';
    *buffer++ = '/';
    *buffer++ = majorVersion + '0';
    *buffer++ = '.';
    *buffer++ = minorVersion + '0';
    *buffer++ = ' ';

    int intCode = static_cast<int>(code);
    *buffer++ = (intCode / 100) + '0';
    *buffer++ = ((intCode / 10) % 10) + '0';
    *buffer++ = (intCode % 100) + '0';
    *buffer++ = ' ';

    auto codeString = getStatusCodeString(code);

    buffer = std::copy(codeString.begin(), codeString.end(), buffer);

    *buffer++ = '\r';
    *buffer++ = '\n';

    buffer = std::copy(simpleResponseHeaders.begin(), simpleResponseHeaders.end(), buffer);

    *buffer++ = '\r';
    *buffer++ = '\n';

    return buffer - originalBufferStart;
}

HttpResponseBuilder::HttpResponseBuilder(WriteFn writeFn, void* writeState)
    : writeFn_(writeFn), writeState_(writeState)
{
    assert(writeFn);
}

void HttpResponseBuilder::writeResponseCode(int majorVersion, int minorVersion, HttpStatusCode code)
{
    assert(ProtocolState::NothingWritten == protocolState_);
    assert(1 == majorVersion);
    assert((0 == minorVersion) || (1 == minorVersion));

    char buffer[] = "HTTP/x.y zzz ";

    buffer[5] = majorVersion + '0';
    buffer[7] = minorVersion + '0';

    const int intCode = static_cast<int>(code);
    buffer[9] = (intCode / 100) + '0';
    buffer[10] = ((intCode / 10) % 10) + '0';
    buffer[11] = (intCode % 100) + '0';

    write(buffer);
    write(getStatusCodeString(code));
    write("\r\n");

    protocolState_ = ProtocolState::ResponseCodeWritten;
}

void HttpResponseBuilder::writeResponseCode(const HttpRequest& request, HttpStatusCode code)
{
    writeResponseCode(request.versionMajor, request.versionMinor, code);
}

void HttpResponseBuilder::writeHeader(HttpStringView name, HttpStringView value)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    write(name);
    write(": ");
    write(value);
    write("\r\n");
}

void HttpResponseBuilder::writeHeader(HttpStringView name, int value)
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

void HttpResponseBuilder::writeCookie(HttpStringView name, HttpStringView value, CookieExtra extra)
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
        *buffer++ = ';';
        *buffer++ = ' ';
        *buffer++ = 'E';
        *buffer++ = 'x';
        *buffer++ = 'p';
        *buffer++ = 'i';
        *buffer++ = 'r';
        *buffer++ = 'e';
        *buffer++ = 's';
        *buffer++ = '=';

        buffer += writeHttpDateGMT(*extra.expires, buffer);
    }

    if (extra.cookieMaxAge)
    {
        *buffer++ = ';';
        *buffer++ = ' ';
        *buffer++ = 'M';
        *buffer++ = 'a';
        *buffer++ = 'x';
        *buffer++ = '-';
        *buffer++ = 'A';
        *buffer++ = 'g';
        *buffer++ = 'e';
        *buffer++ = '=';

        buffer += sprintf(buffer, "%u", *extra.cookieMaxAge);
    }

    if (extra.cookieDomain.length())
    {
        *buffer++ = ';';
        *buffer++ = ' ';
        *buffer++ = 'D';
        *buffer++ = 'o';
        *buffer++ = 'm';
        *buffer++ = 'a';
        *buffer++ = 'i';
        *buffer++ = 'n';
        *buffer++ = '=';

        auto encodedDomain = urlEncode(extra.cookieDomain);
        buffer = std::copy(encodedDomain.begin(), encodedDomain.end(), buffer);
    }

    if (extra.cookiePath.length())
    {
        *buffer++ = ';';
        *buffer++ = ' ';
        *buffer++ = 'P';
        *buffer++ = 'a';
        *buffer++ = 't';
        *buffer++ = 'h';
        *buffer++ = '=';

        auto encodedPath = percentEncode(extra.cookiePath, ReservedCharactersForUrlEncodingWithoutSlash);
        buffer = std::copy(encodedPath.begin(), encodedPath.end(), buffer);
    }

    if (extra.isSecure)
    {
        *buffer++ = ';';
        *buffer++ = ' ';
        *buffer++ = 'S';
        *buffer++ = 'e';
        *buffer++ = 'c';
        *buffer++ = 'u';
        *buffer++ = 'r';
        *buffer++ = 'e';
    }

    if (extra.isHttpOnly)
    {
        *buffer++ = ';';
        *buffer++ = ' ';
        *buffer++ = 'H';
        *buffer++ = 't';
        *buffer++ = 't';
        *buffer++ = 'p';
        *buffer++ = 'O';
        *buffer++ = 'n';
        *buffer++ = 'l';
        *buffer++ = 'y';
    }

    *buffer++ = '\r';
    *buffer++ = '\n';

    write(extraBuffer, buffer - extraBuffer);
}

void HttpResponseBuilder::writeBody(HttpStringView value)
{
    writeBody(value, {});
}

void HttpResponseBuilder::writeBody(HttpStringView value, HttpStringView prefix)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    write("\r\n");
    write(prefix.data(), prefix.size());
    write(value.data(), value.size());

    protocolState_ = ProtocolState::BodyWritten;
}

void HttpResponseBuilder::writeBodyAndContentLength(HttpStringView value)
{
    writeBodyAndContentLength(value, {});
}

void HttpResponseBuilder::writeBodyAndContentLength(HttpStringView value, HttpStringView prefix)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);
    writeHeader("Content-Length", static_cast<int>(value.size() + prefix.size()));
    writeBody(value, prefix);
}
