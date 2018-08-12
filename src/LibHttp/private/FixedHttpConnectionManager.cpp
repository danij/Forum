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

#include "FixedHttpConnectionManager.h"

using namespace Http;

FixedHttpConnectionManager::FixedHttpConnectionManager(std::unique_ptr<HttpRouter>&& httpRouter,
    const size_t numberOfReadBuffers, const size_t numberOfWriteBuffers, const bool trustIpFromXForwardedFor) :

    connectionPool_{ numberOfReadBuffers }, httpRouter_{ std::move(httpRouter) }, 
    readBuffers_{ std::make_unique<HttpConnection::ReadBufferPoolType>(numberOfReadBuffers) },
    writeBuffers_{ std::make_unique<HttpConnection::WriteBufferPoolType>(numberOfWriteBuffers) },
    trustIpFromXForwardedFor_{ trustIpFromXForwardedFor }
{}

IConnectionManager::ConnectionIdentifier FixedHttpConnectionManager::newConnection(IConnectionManager* manager,
    boost::asio::ip::tcp::socket&& socket)
{
    ConnectionIdentifier result = nullptr;

    manager = manager ? manager : this;

    auto closeConnection = true;

    auto headerBuffer = readBuffers_->leaseBuffer();

    if (headerBuffer)
    {
        auto connection = connectionPool_.getObject(*manager, *httpRouter_, socket,
            std::move(headerBuffer), *readBuffers_, *writeBuffers_, trustIpFromXForwardedFor_);
        if (nullptr != connection)
        {
            closeConnection = false;
            result = connection;
            connection->startReading();
        }
    }

    if (closeConnection)
    {
        //socket was not moved if getObject was not called or failed
        closeSocket(socket);
    }

    return result;
}

void FixedHttpConnectionManager::closeConnection(const ConnectionIdentifier identifier)
{
    connectionPool_.returnObject(reinterpret_cast<HttpConnection*>(identifier));
}

void FixedHttpConnectionManager::disconnectConnection(const ConnectionIdentifier identifier)
{
    reinterpret_cast<HttpConnection*>(identifier)->disconnect();
}
