#include "HttpParser.h"

using namespace Http;

/**
 * Matches a string against another one
 * @pre source must point to size characters, against must point to 2 * size characters
 * @param against A string where each character appears as both upper and lower case (e.g. HhEeLlLlO  WwOoRrLlDd)
 */
static bool matchStringUpperOrLower(const char* source, size_t size, const char* against)
{
    char result = 0;
    for (size_t iSource = 0, iAgainst = 0; iSource < size; ++iSource, iAgainst += 2)
    {
        result |= (source[iSource] ^ against[iAgainst]) & (source[iSource] ^ against[iAgainst + 1]);
    }
    return result == 0;
}

Parser::Parser(char* headerBuffer, size_t headerBufferSize, PushBodyBytesFn pushBodyBytes, void* pushBodyBytesState)
    : headerBuffer_(headerBuffer), headerBufferSize_(headerBufferSize), headerSize_(0), pushBodyBytes_(pushBodyBytes),
      pushBodyBytesState_(pushBodyBytesState), valid_(true), finished_(false), currentParser_(&Parser::parseVerb),
      parsePathStartsAt_(headerBuffer_), parseVersionStartsAt_(headerBuffer_), parseHeaderNameStartsAt_(nullptr), 
      parseHeaderValueStartsAt_(nullptr), expectedContentLength_(0)
{
}

Parser& Parser::process(char* buffer, size_t size)
{
    if (finished_)
    {
        //no more parsing necessary
        return *this;
    }
    if ( ! valid_)
    {
        //once the input contains errors, it will always contain them
        return *this;
    }

    (this->*currentParser_)(buffer, size);
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
static bool copyUntil(char toSearch, char*& buffer, size_t& size, bool& valid,
                      char* headerBuffer, size_t& headerSize, size_t headerBufferSize)
{
    bool continueLoop = true;
    do
    {
        if (0 == size) return false;
        if (headerSize >= headerBufferSize)
        {
            valid = false;
            return false;
        }
        if (toSearch == *buffer) continueLoop = false;

        headerBuffer[headerSize++] = *buffer++;
        --size;
    }
    while (continueLoop);
    return true;
}

HttpVerb parseHttpVerb(char* buffer, size_t size)
{
    if (3 == size)
    {
        if (matchStringUpperOrLower(buffer, size, "GgEeTt")) return HttpVerb::GET;
        if (matchStringUpperOrLower(buffer, size, "PpUuTt")) return HttpVerb::PUT;
    }
    if (4 == size)
    {
        if (matchStringUpperOrLower(buffer, size, "PpOoSsTt")) return HttpVerb::POST;
    }
    if (5 == size)
    {
        if (matchStringUpperOrLower(buffer, size, "PpAaTtCcHh")) return HttpVerb::PATCH;
    }
    if (6 == size)
    {
        if (matchStringUpperOrLower(buffer, size, "DdEeLlEeTtEe")) return HttpVerb::DELETE;
    }

    return HttpVerb::UNKNOWN;
}

void Parser::parseVerb(char* buffer, size_t size)
{
    if ( ! copyUntil(' ', buffer, size, valid_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    request_.verb = parseHttpVerb(headerBuffer_, headerSize_ - 1);
    if (HttpVerb::UNKNOWN == request_.verb)
    {
        valid_ = false;
        return;
    }

    currentParser_ = &Parser::parsePath;
    parsePathStartsAt_ = headerBuffer_ + headerSize_;
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parsePath(char* buffer, size_t size)
{
    if ( ! copyUntil(' ', buffer, size, valid_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    request_.path = StringView(parsePathStartsAt_, headerBuffer_ + headerSize_ - 1 - parsePathStartsAt_);

    currentParser_ = &Parser::parseVersion;
    parseVersionStartsAt_ = headerBuffer_ + headerSize_;
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parseVersion(char* buffer, size_t size)
{
    if ( ! copyUntil('\r', buffer, size, valid_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    auto versionSize = headerBuffer_ + headerSize_ - 1 - parseVersionStartsAt_;
    if (versionSize != 8) //e.g. HTTP/1.1
    {
        valid_ = false;
        return;
    }
    if (parseVersionStartsAt_[5] != '1')
    {
        valid_ = false;
        return;
    }
    request_.versionMajor = 1;
    if (parseVersionStartsAt_[7] == '0')
    {
        request_.versionMinor = 0;
    }
    if (parseVersionStartsAt_[7] == '1')
    {
        request_.versionMinor = 1;
    }
    else
    {
        valid_ = false;
        return;
    }
    currentParser_ = &Parser::parseNewLine;
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parseNewLine(char* buffer, size_t size)
{
    if (0 == size) return;
    //\r was already added by previous steps
    if (('\n' != *buffer++) || (headerSize_ >= headerBufferSize_) || 
        (headerSize_ < 1) || (headerBuffer_[headerSize_ - 1] != '\r'))
    {
        valid_ = false;
        return;
    }
    headerBuffer_[headerSize_++] = '\n';
    --size;

    if ((headerSize_ > 4) && (headerBuffer_[headerSize_ - 3] == '\n') && (headerBuffer_[headerSize_ - 4] == '\r'))
    {
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
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parseHeaderName(char* buffer, size_t size)
{
    if (size && *buffer == '\r')
    {
        if (headerSize_ >= headerBufferSize_)
        {
            valid_ = false;
            return;
        }
        ++buffer;
        --size;
        headerBuffer_[headerSize_++] = '\r';
        currentParser_ = &Parser::parseNewLine;
    }
    else
    {
        if ( ! copyUntil(':', buffer, size, valid_, headerBuffer_, headerSize_, headerBufferSize_)) return;

        parseCurrentHeaderName_ = StringView(parseHeaderNameStartsAt_, 
                                             headerBuffer_ + headerSize_ - 1 - parseHeaderNameStartsAt_);

        currentParser_ = &Parser::parseHeaderSpacing;
    }
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parseHeaderSpacing(char* buffer, size_t size)
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
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parseHeaderValue(char* buffer, size_t size)
{
    if ( ! copyUntil('\r', buffer, size, valid_, headerBuffer_, headerSize_, headerBufferSize_)) return;

    parseCurrentHeaderValue_ = StringView(parseHeaderValueStartsAt_, 
                                          headerBuffer_ + headerSize_ - 1 - parseHeaderValueStartsAt_);

    currentParser_ = &Parser::parseNewLine;
    if (size)
    {
        (this->*currentParser_)(buffer, size);
    }
}

void Parser::parseBody(char* buffer, size_t size)
{
    //TODO: chunked encoding
    if ( ! pushBodyBytes_(buffer, size, pushBodyBytesState_))
    {
        //no more room to store the request body
        valid_ = false;
    }
}
