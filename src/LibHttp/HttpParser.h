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

#pragma once

#include "HttpConstants.h"
#include "HttpRequest.h"

#include <boost/noncopyable.hpp>

namespace Http
{
    class Parser final : boost::noncopyable
    {
    public:
        //returns true if there is still room for more bytes
        typedef bool(*PushBodyBytesFn)(const char* buffer, size_t bufferSize, void* state);

        explicit Parser(char* headerBuffer, size_t headerBufferSize, size_t maxContentLength,
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
        HttpStringView parseCurrentHeaderName_;
        char* parseHeaderValueStartsAt_;
        HttpStringView parseCurrentHeaderValue_;
        size_t expectedContentLength_ = 0;
        size_t maxContentLength_;
        size_t requestBodyBytesProcessed_ = 0;
    };
}
