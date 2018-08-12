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

#include <boost/asio.hpp>

namespace Http
{
    class IConnectionManager
    {
    public:
        IConnectionManager() = default;
        virtual ~IConnectionManager() = default;
        
        IConnectionManager(const IConnectionManager& other) = delete;
        IConnectionManager(IConnectionManager&& other) = default;
        IConnectionManager& operator=(const IConnectionManager& other) = delete;
        IConnectionManager& operator=(IConnectionManager&& other) = default;

        using ConnectionIdentifier = void*;

        virtual ConnectionIdentifier newConnection(IConnectionManager* manager, boost::asio::ip::tcp::socket&& socket) = 0;
        virtual void closeConnection(ConnectionIdentifier identifier) = 0;
        virtual void disconnectConnection(ConnectionIdentifier identifier) = 0;
        virtual void stop() {}
    };
}