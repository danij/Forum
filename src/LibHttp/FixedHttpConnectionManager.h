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

#include "IConnectionManager.h"
#include "FixedSizeObjectPool.h"
#include "HttpConnection.h"
#include "HttpRouter.h"

#include <atomic>
#include <cstdint>

namespace Http
{
    class FixedHttpConnectionManager : public IConnectionManager
    {
    public:
        FixedHttpConnectionManager(std::unique_ptr<HttpRouter>&& httpRouter,
                                   size_t numberOfReadBuffers, size_t numberOfWriteBuffers, 
                                   bool trustIpFromXForwardedFor);

        ConnectionIdentifier newConnection(IConnectionManager* manager, boost::asio::ip::tcp::socket&& socket) override;
        void closeConnection(ConnectionIdentifier identifier) override;

    private:
        FixedSizeObjectPool<HttpConnection> connectionPool_;

        std::unique_ptr<HttpRouter> httpRouter_;
        std::unique_ptr<HttpConnection::ReadBufferPoolType> readBuffers_;
        std::unique_ptr<HttpConnection::WriteBufferPoolType> writeBuffers_;
        bool trustIpFromXForwardedFor_;
    };
}
