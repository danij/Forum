#include "HttpResponseBuilder.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

using namespace Http;

static const StringView simpleResponseHeaders{"Connection: close\r\nContent-Length: 0\r\n"};

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
    
    int intCode = static_cast<int>(code);
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

void HttpResponseBuilder::writeHeader(StringView name, StringView value)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    write(name);
    write(": ");
    write(value);
    write("\r\n");
}

void HttpResponseBuilder::writeHeader(StringView name, int value)
{
    char buffer[50];
    auto written = sprintf(buffer, "%d", value);
    writeHeader(name, StringView(buffer, written));
}

void HttpResponseBuilder::writeCookie(StringView name, StringView value, CookieExtra extra)
{
    //TODO
}

void HttpResponseBuilder::writeBody(const char* value, size_t length)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);

    write("\r\n");
    write(value, length);

    protocolState_ = ProtocolState::BodyWritten;
}

void HttpResponseBuilder::writeBodyAndContentLength(const char* value, size_t length)
{
    assert(ProtocolState::ResponseCodeWritten == protocolState_);
    writeHeader("Content-Length", length);
    writeBody(value, length);
}
