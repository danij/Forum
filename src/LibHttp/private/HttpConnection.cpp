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

#include "HttpConnection.h"

using namespace Http;

static void writeToBuffer(const char* data, const size_t size, void* state)
{
    reinterpret_cast<HttpConnection::ResponseBufferType*>(state)->write(data, size);
}

HttpConnection::HttpConnection(IConnectionManager& connectionManager, HttpRouter& router,
    boost::asio::ip::tcp::socket& socket, boost::asio::io_context& context,
    ReadBufferType&& headerBuffer, ReadBufferPoolType& readBufferPool, WriteBufferPoolType& writeBufferPool,
    const bool trustIpFromXForwardedFor) :

    StreamingConnection(connectionManager, std::move(socket), context), router_{ router },
    headerBuffer_(std::move(headerBuffer)), 
    requestBodyBuffer_(readBufferPool), 
    responseBuffer_(writeBufferPool),
    responseBuilder_(writeToBuffer, &responseBuffer_),
    trustIpFromXForwardedFor_(trustIpFromXForwardedFor),
    parser_(headerBuffer_->data, Buffer::ReadBufferSize, Buffer::MaxRequestBodyLength,
            [](auto buffer, auto size, auto state)
            {
                return reinterpret_cast<HttpConnection*>(state)->onReadBody(buffer, size);
            }, this)
{}

bool HttpConnection::onBytesRead(char* bytes, size_t bytesTransfered)
{
    if (parser_.process(bytes, bytesTransfered) == Parser::ParseResult::INVALID_INPUT)
    {
        //invalid input
        writeStatusCode(parser_.errorCode());
        return false;
    }

    if (parser_ != Parser::ParseResult::FINISHED) return true;

    //finished reading everything needed for the current request, so process it
    auto& request = parser_.mutableRequest();

    processRequest(request);

    //don't restart reading for the moment
    return false;
}

void HttpConnection::onWritten(size_t /*bytesTransfered*/)
{
    if (keepConnectionAlive_)
    {
        parser_.reset();
        requestBodyBuffer_.reset();
        responseBuffer_.reset();
        responseBuilder_.reset();
        startReading();
    }
    else
    {
        release();
    }
}

bool HttpConnection::onReadBody(const char* buffer, const size_t size)
{
    return requestBodyBuffer_.write(buffer, size);
}

void HttpConnection::writeStatusCode(const HttpStatusCode code)
{
    //reuse the input buffer for sending the error code
    const auto responseSize = buildSimpleResponseFromStatusCode(code,
            parser_.request().versionMajor, parser_.request().versionMinor, readBuffer_.data());

    write(boost::asio::buffer(readBuffer_.data(), responseSize));
}

void HttpConnection::processRequest(HttpRequest& request)
{
    keepConnectionAlive_ = request.keepConnectionAlive;

    //add references to request content buffers
    for (const auto buffer : requestBodyBuffer_.constBufferWrapper())
    {
        if (request.nrOfRequestContentBuffers >= request.requestContentBuffers.size())
        {
            break;
        }
        request.requestContentBuffers[request.nrOfRequestContentBuffers++] =
            HttpStringView(boost::asio::buffer_cast<const char*>(buffer), boost::asio::buffer_size(buffer));
    }

    //add remote endpoint
    request.remoteAddress = getRemoteAddress(request);
    
    router_.forward(request, responseBuilder_);

    if ((0 == responseBuffer_.size()) || responseBuffer_.notEnoughRoom())
    {
        writeStatusCode(HttpStatusCode::Internal_Server_Error);
    }
    else
    {
        write(responseBuffer_.constBufferWrapper());
    }
}

boost::asio::ip::address HttpConnection::getRemoteAddress(const HttpRequest& request)
{
    if (trustIpFromXForwardedFor_)
    {
        char nullTerminatedAddressBuffer[128];

        auto xForwardedFor = request.headers[Http::Request::X_Forwarded_For];
        const auto toCopy = std::min(std::size(nullTerminatedAddressBuffer) - 1, xForwardedFor.size());
        std::copy(xForwardedFor.data(), xForwardedFor.data() + toCopy, nullTerminatedAddressBuffer);
        nullTerminatedAddressBuffer[toCopy] = 0;

        boost::system::error_code parseAddressCode;
        const auto address = boost::asio::ip::address::from_string(nullTerminatedAddressBuffer, parseAddressCode);
        if ( ! parseAddressCode)
        {
            return address;
        }
    }
    else
    {
        boost::system::error_code getAddressCode;
        auto remoteEndpoint = socket_.remote_endpoint(getAddressCode);
        if ( ! getAddressCode)
        {
            return remoteEndpoint.address();
        }
    }
    return {};
}
