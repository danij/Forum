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

#include "HttpParser.h"
#include "HttpStringHelpers.h"

#include <cassert>
#include <limits>

#include <boost/lexical_cast/try_lexical_convert.hpp>

using namespace Http;

Parser::Parser(char* headerBuffer, const size_t headerBufferSize, const size_t maxContentLength,
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
    errorCode_ = HttpStatusCode::Bad_Request;
    currentParser_ = &Parser::parseVerb;
    parsePathStartsAt_ = {};
    parseVersionStartsAt_ = {};
    parseHeaderNameStartsAt_ = {};
    parseCurrentHeaderName_ = {};
    parseHeaderValueStartsAt_ = {};
    parseCurrentHeaderValue_ = {};
    expectedContentLength_ = 0;
    requestBodyBytesProcessed_ = 0;
}

/**
 * Copies characters until it finds a specific char + that character
 * Checks if there is enough room in the buffer
 * @return true if the character was reached, false otherwise
 */
static bool copyUntil(const char toSearch, char*& buffer, size_t& size, bool& valid, HttpStatusCode& errorCode,
                      char* headerBuffer, size_t& headerSize, const size_t headerBufferSize)
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

static HttpVerb parseHttpVerb(char* buffer, const size_t size)
{
    switch (size)
    {
    case 3:
        if (matchStringUpperOrLowerSameSize(buffer, "getGET")) return HttpVerb::GET;
        if (matchStringUpperOrLowerSameSize(buffer, "putPUT")) return HttpVerb::PUT;
        break;
    case 4:
        if (matchStringUpperOrLowerSameSize(buffer, "postPOST")) return HttpVerb::POST;
        break;
    case 5:
        if (matchStringUpperOrLowerSameSize(buffer, "patchPATCH")) return HttpVerb::PATCH;
        break;
    case 6:
        if (matchStringUpperOrLowerSameSize(buffer, "deleteDELETE")) return HttpVerb::DELETE;
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

    request_.path = HttpStringView(parsePathStartsAt_, headerBuffer_ + headerSize_ - parsePathStartsAt_);
    interpretPathString();
    trimLeadingChar(request_.path, '/');

    currentParser_ = &Parser::parseVersion;
    parseVersionStartsAt_ = headerBuffer_ + headerSize_;
}

void Parser::parseVersion(char*& buffer, size_t& size)
{
    if ( ! copyUntil('\r', buffer, size, valid_, errorCode_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    const auto versionSize = headerBuffer_ + headerSize_ - 1 - parseVersionStartsAt_;
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
        if (request_.verb == HttpVerb::GET || request_.verb == HttpVerb::DELETE || (expectedContentLength_ < 1))
        {
            finished_ = true;
            return;
        }

        if ( ! request_.headers[Request::HttpHeader::Transfer_Encoding].empty()
            || ! request_.headers[Request::HttpHeader::Content_Encoding].empty())
        {
            valid_ = false;
            errorCode_ = HttpStatusCode::Not_Implemented;
            return;
        }
        currentParser_ = &Parser::parseBody;
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

        parseCurrentHeaderName_ = HttpStringView(parseHeaderNameStartsAt_,
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

    parseCurrentHeaderValue_ = HttpStringView(parseHeaderValueStartsAt_,
                                              headerBuffer_ + headerSize_ - 1 - parseHeaderValueStartsAt_);

    currentParser_ = &Parser::parseNewLine;
    const auto currentHeader = Request::matchHttpHeader(parseCurrentHeaderName_.data(), parseCurrentHeaderName_.size());
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
    auto contentLengthString = request_.headers[Request::HttpHeader::Content_Length];

    if ( ! boost::conversion::try_lexical_convert(contentLengthString.data(), contentLengthString.size(), expectedContentLength_))
    {
        expectedContentLength_ = 0;
    }

    auto connectionHeaderString = request_.headers[Request::HttpHeader::Connection];
    request_.keepConnectionAlive = matchStringUpperOrLower(connectionHeaderString.data(),
                                                           connectionHeaderString.size(),
                                                           "keep-aliveKEEP-ALIVE");

    if ( ! request_.headers[Request::HttpHeader::Expect].empty())
    {
        //no need to support such requests for the moment
        valid_ = false;
        errorCode_ = HttpStatusCode::Expectation_Failed;
    }

    auto cookieValue = request_.headers[Request::HttpHeader::Cookie];
    interpretCookies(const_cast<char*>(cookieValue.data()), static_cast<int>(cookieValue.size()));
}

void Parser::interpretPathString()
{
    int state = 0;
    const auto pathStart = parsePathStartsAt_;
    int keyStart = 0, keyEnd = 0, valueStart = 0, valueEnd = 0;

    auto fullPath = request_.path;

    for (int i = 0, n = static_cast<int>(fullPath.size()); i < n; ++i)
    {
        const auto c = fullPath[i];
        switch (state)
        {
        case 0: //path
            if (('?' == c) || ((i + 1) == n))
            {
                request_.path = viewAfterDecodingUrlEncodingInPlace(parsePathStartsAt_, i);
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
                        viewAfterDecodingUrlEncodingInPlace(keyEnd > keyStart ? pathStart + keyStart : nullptr,
                                                            keyEnd - keyStart);
                request_.queryPairs[request_.nrOfQueryPairs].second =
                        viewAfterDecodingUrlEncodingInPlace(valueEnd > valueStart ? pathStart + valueStart : nullptr,
                                                            valueEnd - valueStart);
                request_.nrOfQueryPairs += 1;
                keyStart = keyEnd = i + 1;
                state = 1;
            }
            break;
        default: 
            assert(false);
            break;
        }
    }
}

void Parser::interpretCookies(char* value, const size_t size)
{
    int state = 0;
    const auto cookieStart = value;
    int nameStart = 0, nameEnd = 0, valueStart = 0, valueEnd = 0;

    for (size_t i = 0; i < size; ++i)
    {
        const auto c = value[i];

        switch (state)
        {
        case 0: //name
            if ('=' == c)
            {
                nameEnd = i;
                state = 1;
                valueStart = valueEnd = i + 1;
            }
            else if ((';' == c) || ((i + 1) == size))
            {
                //no name, just a value
                valueEnd = i;
                if ((i + 1) == size)
                {
                    valueEnd += 1;
                }
                while (cookieStart[valueStart] == ' ') valueStart += 1;
                while (cookieStart[valueEnd - 1] == ' ') valueEnd -= 1;

                request_.cookies[request_.nrOfCookies].first = {};
                request_.cookies[request_.nrOfCookies].second =
                    viewAfterDecodingUrlEncodingInPlace(valueEnd > valueStart ? cookieStart + valueStart : nullptr,
                        valueEnd - valueStart);

                request_.nrOfCookies += 1;
                nameStart = nameEnd = valueStart = valueEnd = i + 1;
                state = 0;
            }
            break;
        case 1: //value
            if ((';' == c) || ((i + 1) == size))
            {
                valueEnd = i;
                if ((i + 1) == size)
                {
                    valueEnd += 1;
                }
                while (cookieStart[nameStart] == ' ') nameStart += 1;
                while (cookieStart[nameEnd - 1] == ' ') nameEnd -= 1;
                while (cookieStart[valueStart] == ' ') valueStart += 1;
                while (cookieStart[valueEnd - 1] == ' ') valueEnd -= 1;

                request_.cookies[request_.nrOfCookies].first =
                        viewAfterDecodingUrlEncodingInPlace(nameEnd > nameStart ? cookieStart + nameStart : nullptr,
                                                            nameEnd - nameStart);
                request_.cookies[request_.nrOfCookies].second =
                        viewAfterDecodingUrlEncodingInPlace(valueEnd > valueStart ? cookieStart + valueStart : nullptr,
                                                            valueEnd - valueStart);
                request_.nrOfCookies += 1;
                nameStart = nameEnd = valueStart = valueEnd = i + 1;
                state = 0;
            }
            break;
        default: 
            assert(false);
            break;
        }
    }
}
