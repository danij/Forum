/*
Fast Forum Backend
Copyright (C) Daniel Jurcau

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

#include "StreamingConnection.h"
#include "FixedSizeBufferPool.h"
#include "ReadWriteBufferArray.h"
#include "HttpConstants.h"
#include "HttpParser.h"
#include "HttpResponseBuilder.h"
#include "HttpRouter.h"

namespace Http
{
    class HttpConnection : public StreamingConnection
    {
    public:
        typedef FixedSizeBufferPool<Buffer::ReadBufferSize> ReadBufferPoolType;
        typedef FixedSizeBufferPool<Buffer::ReadBufferSize>::LeasedBufferType ReadBufferType;
        typedef FixedSizeBufferPool<Buffer::WriteBufferSize> WriteBufferPoolType;
        typedef FixedSizeBufferPool<Buffer::WriteBufferSize>::LeasedBufferType WriteBufferType;
        typedef ReadWriteBufferArray<Buffer::ReadBufferSize, Buffer::MaximumBuffersForRequestBody> RequestBodyBufferType;
        typedef ReadWriteBufferArray<Buffer::WriteBufferSize, Buffer::MaximumBuffersForResponse> ResponseBufferType;
        
        explicit HttpConnection(IConnectionManager& connectionManager, HttpRouter& router, 
            boost::asio::ip::tcp::socket& socket,
            ReadBufferType&& headerBuffer, ReadBufferPoolType& readBufferPool, WriteBufferPoolType& writeBufferPool,
            bool trustIpFromXForwardedFor);

    protected:
        bool onBytesRead(char* bytes, size_t bytesTransfered) override;
        void onWritten(size_t bytesTransfered) override;

    private:
        bool onReadBody(const char* buffer, size_t size);
        void writeStatusCode(HttpStatusCode code);
        void processRequest(HttpRequest& request);
        boost::asio::ip::address getRemoteAddress(const HttpRequest& request);

        HttpRouter& router_;
        ReadBufferType headerBuffer_;
        RequestBodyBufferType requestBodyBuffer_;
        ResponseBufferType responseBuffer_;
        HttpResponseBuilder responseBuilder_;
        bool keepConnectionAlive_ = false;
        bool trustIpFromXForwardedFor_ = false;
        Parser parser_;
    };
}
