#pragma once

#include "HttpConstants.h"

#include <cstddef>
#include <cstdint>

#include <boost/noncopyable.hpp>
#include <boost/utility/string_view.hpp>

namespace Http
{
    typedef boost::string_view StringView;

#ifdef DELETE
#undef DELETE
#endif

    enum class HttpVerb
    {
        UNKNOWN,
        GET,
        POST,
        PUT,
        PATCH,
        DELETE
    };   

    struct HttpRequest
    {//StringViews point to addresses inside the header buffer
        HttpVerb verb = HttpVerb::UNKNOWN;
        StringView path;
        int_fast8_t versionMajor = 1, versionMinor = 0;
        bool keepConnectionAlive = false;
        StringView headers[HttpHeader::HTTP_HEADERS_COUNT];
    };

    class Parser final : private boost::noncopyable
    {
    public:
        //returns true if there is still room for more bytes
        typedef bool(*PushBodyBytesFn)(char* buffer, size_t bufferSize, void* state);

        explicit Parser(char* headerBuffer, size_t headerBufferSize, PushBodyBytesFn pushBodyBytes, 
                        void* pushBodyBytesState);

        enum ParseResult
        {
            INVALID_INPUT,
            ONGOING,
            FINISHED,
        };

        Parser& process(char* buffer, size_t size);

        /**
         * Resets the state of the parser, making it ready to start processing a new request
         */
        void reset();

        operator ParseResult() const
        {
            return valid_
                ? (finished_ ? FINISHED : ONGOING)
                : INVALID_INPUT;
        }

        const HttpRequest& request() const
        {
            return request_;
        }

    private:
        
        typedef void (Parser::*ParserFn)(char* buffer, size_t size);

        void parseVerb(char* buffer, size_t size);
        void parsePath(char* buffer, size_t size);
        void parseVersion(char* buffer, size_t size);
        void parseNewLine(char* buffer, size_t size);
        void parseHeaderName(char* buffer, size_t size);
        void parseHeaderSpacing(char* buffer, size_t size);
        void parseHeaderValue(char* buffer, size_t size);
        void parseBody(char* buffer, size_t size);

        char* headerBuffer_;
        size_t headerBufferSize_;
        size_t headerSize_ = 0;
        PushBodyBytesFn pushBodyBytes_;
        void* pushBodyBytesState_;
        HttpRequest request_;

        bool valid_ = true;
        bool finished_ = false;
        ParserFn currentParser_ = &Parser::parseVerb;
        char* parsePathStartsAt_;
        char* parseVersionStartsAt_;
        char* parseHeaderNameStartsAt_;
        StringView parseCurrentHeaderName_;
        char* parseHeaderValueStartsAt_;
        StringView parseCurrentHeaderValue_;
        size_t expectedContentLength_ = 0;
    };
}
