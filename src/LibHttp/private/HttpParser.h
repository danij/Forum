#pragma once

#include "HttpConstants.h"
#include "HttpRequest.h"

#include <boost/noncopyable.hpp>
#include <boost/utility/string_view.hpp>

namespace Http
{
    class Parser final : private boost::noncopyable
    {
    public:
        //returns true if there is still room for more bytes
        typedef bool(*PushBodyBytesFn)(const char* buffer, size_t bufferSize, void* state);

        explicit Parser(char* headerBuffer, size_t headerBufferSize, size_t maxBodyLength,
                        PushBodyBytesFn pushBodyBytes, void* pushBodyBytesState);

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

        HttpStatusCode errorCode() const
        {
            return errorCode_;
        }

        const HttpRequest& request() const
        {
            return request_;
        }

        HttpRequest& mutableRequest()
        {
            return request_;
        }

    private:

        typedef void (Parser::*ParserFn)(char*& buffer, size_t& size);

        void          parseVerb(char*& buffer, size_t& size);
        void          parsePath(char*& buffer, size_t& size);
        void       parseVersion(char*& buffer, size_t& size);
        void       parseNewLine(char*& buffer, size_t& size);
        void    parseHeaderName(char*& buffer, size_t& size);
        void parseHeaderSpacing(char*& buffer, size_t& size);
        void   parseHeaderValue(char*& buffer, size_t& size);
        void          parseBody(char*& buffer, size_t& size);

        void onFinishedParsingHeaders();
        void interpretImportantHeaders();
        void interpretPathString();
        void interpretCookies(char* value, size_t size);

        char* headerBuffer_;
        size_t headerBufferSize_;
        size_t headerSize_ = 0;
        PushBodyBytesFn pushBodyBytes_;
        void* pushBodyBytesState_;
        HttpRequest request_;

        bool valid_ = true;
        bool finished_ = false;
        HttpStatusCode errorCode_ = HttpStatusCode::Bad_Request;
        ParserFn currentParser_ = &Parser::parseVerb;
        char* parsePathStartsAt_;
        char* parseVersionStartsAt_;
        char* parseHeaderNameStartsAt_;
        StringView parseCurrentHeaderName_;
        char* parseHeaderValueStartsAt_;
        StringView parseCurrentHeaderValue_;
        size_t expectedContentLength_ = 0;
        size_t maxContentLength_;
        size_t requestBodyBytesProcessed_ = 0;
    };
}
