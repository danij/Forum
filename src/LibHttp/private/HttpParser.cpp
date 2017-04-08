#include "HttpParser.h"
#include "StringHelpers.h"

#include <cassert>
#include <boost/lexical_cast/try_lexical_convert.hpp>

using namespace Http;

Parser::Parser(char* headerBuffer, size_t headerBufferSize, size_t maxContentLength,
               PushBodyBytesFn pushBodyBytes, void* pushBodyBytesState)
    : headerBuffer_(headerBuffer), headerBufferSize_(headerBufferSize), pushBodyBytes_(pushBodyBytes),
      pushBodyBytesState_(pushBodyBytesState), parsePathStartsAt_(headerBuffer_), 
      parseVersionStartsAt_(headerBuffer_), parseHeaderNameStartsAt_(headerBuffer_),
      parseHeaderValueStartsAt_(headerBuffer_), maxContentLength_(maxContentLength)
{
    assert(headerBuffer);
    assert(pushBodyBytes);
}

Parser& Parser::process(char* buffer, size_t size)
{
    if (finished_)
    {
        //no more parsing necessary
        return *this;
    }
    while ((size > 0) && valid_ && ! finished_)//once the input contains errors, it will always contain them
    {
        (this->*currentParser_)(buffer, size);
    }
    return *this;
}

void Parser::reset()
{
    headerSize_ = {};
    request_ = {};
    valid_ = true;
    finished_ = false;
    currentParser_ = &Parser::parseVerb;
    parsePathStartsAt_ = {};
    parseVersionStartsAt_ = {};
    parseHeaderNameStartsAt_ = {};
    parseCurrentHeaderName_ = {};
    parseHeaderValueStartsAt_ = {};
    parseCurrentHeaderValue_ = {};
    expectedContentLength_ = {};
}

/**
 * Copies characters until it finds a specific char + that character
 * Checks if there is enough room in the buffer
 * @return true if the character was reached, false otherwise
 */
static bool copyUntil(char toSearch, char*& buffer, size_t& size, bool& valid, HttpStatusCode& errorCode,
                      char* headerBuffer, size_t& headerSize, size_t headerBufferSize)
{
    bool continueLoop = true;
    do
    {
        if (0 == size) return false;
        if (headerSize >= headerBufferSize)
        {
            valid = false;
            errorCode = HttpStatusCode::Payload_Too_Large;
            return false;
        }
        if (toSearch == *buffer) continueLoop = false;

        headerBuffer[headerSize++] = *buffer++;
        --size;
    }
    while (continueLoop);
    return true;
}

static HttpVerb parseHttpVerb(char* buffer, size_t size)
{
    switch (size)
    {
    case 3:
        if (matchStringUpperOrLower(buffer, "GgEeTt")) return HttpVerb::GET;
        if (matchStringUpperOrLower(buffer, "PpUuTt")) return HttpVerb::PUT;
        break;
    case 4:
        if (matchStringUpperOrLower(buffer, "PpOoSsTt")) return HttpVerb::POST;
        break;
    case 5:
        if (matchStringUpperOrLower(buffer, "PpAaTtCcHh")) return HttpVerb::PATCH;
        break;
    case 6:
        if (matchStringUpperOrLower(buffer, "DdEeLlEeTtEe")) return HttpVerb::DELETE;
        break;
    }
    return HttpVerb::UNKNOWN;
}

void Parser::parseVerb(char*& buffer, size_t& size)
{
    if ( ! copyUntil(' ', buffer, size, valid_, errorCode_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    request_.verb = parseHttpVerb(headerBuffer_, headerSize_ - 1);
    if (HttpVerb::UNKNOWN == request_.verb)
    {
        valid_ = false;
        return;
    }

    currentParser_ = &Parser::parsePath;
    parsePathStartsAt_ = headerBuffer_ + headerSize_;
}

void Parser::parsePath(char*& buffer, size_t& size)
{
    if ( ! copyUntil(' ', buffer, size, valid_, errorCode_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    request_.path = StringView(parsePathStartsAt_, headerBuffer_ + headerSize_ - 1 - parsePathStartsAt_);
    interpretPathString();

    currentParser_ = &Parser::parseVersion;
    parseVersionStartsAt_ = headerBuffer_ + headerSize_;
}

void Parser::parseVersion(char*& buffer, size_t& size)
{
    if ( ! copyUntil('\r', buffer, size, valid_, errorCode_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    auto versionSize = headerBuffer_ + headerSize_ - 1 - parseVersionStartsAt_;
    if (versionSize != 8) //e.g. HTTP/1.1
    {
        valid_ = false;
        errorCode_ = HttpStatusCode::HTTP_Version_Not_Supported;
        return;
    }
    if (parseVersionStartsAt_[5] != '1')
    {
        valid_ = false;
        errorCode_ = HttpStatusCode::HTTP_Version_Not_Supported;
        return;
    }
    request_.versionMajor = 1;
    if (parseVersionStartsAt_[7] == '0')
    {
        request_.versionMinor = 0;
    }
    else if (parseVersionStartsAt_[7] == '1')
    {
        request_.versionMinor = 1;
    }
    else
    {
        valid_ = false;
        errorCode_ = HttpStatusCode::HTTP_Version_Not_Supported;
        return;
    }
    currentParser_ = &Parser::parseNewLine;
}

void Parser::parseNewLine(char*& buffer, size_t& size)
{
    if (0 == size) return;
    //\r was already added by previous steps
    if (('\n' != *buffer++) || (headerSize_ >= headerBufferSize_) || 
        (headerSize_ < 1) || (headerBuffer_[headerSize_ - 1] != '\r'))
    {
        valid_ = false;
        errorCode_ = HttpStatusCode::Payload_Too_Large;
        return;
    }
    headerBuffer_[headerSize_++] = '\n';
    --size;

    if ((headerSize_ > 4) && (headerBuffer_[headerSize_ - 3] == '\n') && (headerBuffer_[headerSize_ - 4] == '\r'))
    {
        onFinishedParsingHeaders();
        if (request_.verb == HttpVerb::GET || request_.verb == HttpVerb::DELETE)
        {
            finished_ = true;
            return;
        }
        else
        {
            currentParser_ = &Parser::parseBody;
        }
    }
    else
    {
        currentParser_ = &Parser::parseHeaderName;
        parseHeaderNameStartsAt_ = headerBuffer_ + headerSize_;
    }
}

void Parser::parseHeaderName(char*& buffer, size_t& size)
{
    if (size && *buffer == '\r')
    {
        if (headerSize_ >= headerBufferSize_)
        {
            valid_ = false;
            errorCode_ = HttpStatusCode::Payload_Too_Large;
            return;
        }
        ++buffer;
        --size;
        headerBuffer_[headerSize_++] = '\r';
        currentParser_ = &Parser::parseNewLine;
    }
    else
    {
        if ( ! copyUntil(':', buffer, size, valid_, errorCode_, headerBuffer_, headerSize_, headerBufferSize_)) return;

        parseCurrentHeaderName_ = StringView(parseHeaderNameStartsAt_, 
                                             headerBuffer_ + headerSize_ - 1 - parseHeaderNameStartsAt_);

        currentParser_ = &Parser::parseHeaderSpacing;
    }
}

void Parser::parseHeaderSpacing(char*& buffer, size_t& size)
{
    while (true)
    {
        if (0 == size) return;
        if (*buffer == ' ')
        {
            ++buffer;
            --size;
        }
        else
        {
            break;
        }
    }
    currentParser_ = &Parser::parseHeaderValue;
    parseHeaderValueStartsAt_ = headerBuffer_ + headerSize_;
}

void Parser::parseHeaderValue(char*& buffer, size_t& size)
{
    if ( ! copyUntil('\r', buffer, size, valid_, errorCode_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    parseCurrentHeaderValue_ = StringView(parseHeaderValueStartsAt_, 
                                          headerBuffer_ + headerSize_ - 1 - parseHeaderValueStartsAt_);

    currentParser_ = &Parser::parseNewLine;
    auto currentHeader = matchHttpHeader(parseCurrentHeaderName_.data(), parseCurrentHeaderName_.size());
    if (currentHeader)
    {
        request_.headers[currentHeader] = parseCurrentHeaderValue_;
    }
}

void Parser::parseBody(char*& buffer, size_t& size)
{
    //TODO: chunked encoding
    if (expectedContentLength_ > maxContentLength_)
    {
        valid_ = false;
        errorCode_ = HttpStatusCode::Payload_Too_Large;
        return;
    }

    if ( ! pushBodyBytes_(buffer, size, pushBodyBytesState_))
    {
        //no more room to store the request body
        valid_ = false;
        errorCode_ = HttpStatusCode::Payload_Too_Large;
    }
    requestBodyBytesProcessed_ += size;
    size = 0;
    if (requestBodyBytesProcessed_ >= expectedContentLength_)
    {
        finished_ = true;
    }
}

void Parser::onFinishedParsingHeaders()
{
    interpretImportantHeaders();
}

void Parser::interpretImportantHeaders()
{
    expectedContentLength_ = 0;
    auto contentLengthString = request_.headers[HttpHeader::Content_Length];

    if ( ! boost::conversion::try_lexical_convert(contentLengthString.data(), contentLengthString.size(), expectedContentLength_))
    {
        expectedContentLength_ = 0;
    }

    auto connectionHeaderString = request_.headers[HttpHeader::Connection];
    request_.keepConnectionAlive = matchStringUpperOrLower(connectionHeaderString.data(), 
                                                           connectionHeaderString.size(), 
                                                           "kKeEeEpP--aAlLiIvVeE");

    if (request_.headers[HttpHeader::Expect].size())
    {
        //no need to support such requests for the moment
        valid_ = false;
        errorCode_ = HttpStatusCode::Expectation_Failed;
    }
}

void Parser::interpretPathString()
{
    int state = 0;
    auto pathStart = request_.path.data();
    char c;
    int keyStart = 0, keyEnd = 0, valueStart = 0, valueEnd = 0;
    for (int i = 0, n = request_.path.size(); i < n; ++i)
    {
        c = request_.path[i];
        switch (state)
        {
        case 0: //path
            if (('?' == c) || ((i + 1) == n))
            {
                request_.path = StringView(request_.path.data(), i);
                state = 1;
                keyStart = keyEnd = i + 1;
            }
            break;
        case 1: //query string key
            if ('=' == c)
            {
                keyEnd = i;
                state = 2;
                valueStart = valueEnd = i + 1;
            }
            break;
        case 2: //query string value
            if (('&' == c) || ((i + 1) == n))
            {
                valueEnd = i;
                request_.queryPairs[request_.nrOfQueryPairs].first = 
                        StringView(keyEnd > keyStart ? pathStart + keyStart : nullptr, keyEnd - keyStart);
                request_.queryPairs[request_.nrOfQueryPairs].second = 
                        StringView(valueEnd > valueStart ? pathStart + valueStart : nullptr, valueEnd - valueStart);
                request_.nrOfQueryPairs += 1;
                keyStart = keyEnd = i + 1;
                state = 1;
            }
            break;
        }
    }
}
