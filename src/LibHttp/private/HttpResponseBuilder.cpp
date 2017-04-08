#include "HttpResponseBuilder.h"
#include <algorithm>

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
