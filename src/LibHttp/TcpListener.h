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

#include <cstdint>
#include <memory>
#include <string_view>

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

namespace Http
{
    class TcpListener final : boost::noncopyable
    {
    public:
        explicit TcpListener(boost::asio::io_service& ioService, std::string_view listenIpAddress, uint16_t listenPort,
            std::shared_ptr<IConnectionManager> connectionManager);
        ~TcpListener();

        void startListening();
        void stopListening();

    private:

        void startAccept();
        void onAccept(const boost::system::error_code& ec);
        
        boost::asio::ip::address listenIpAddress_;
        uint16_t listenPort_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::ip::tcp::socket currentSocket_;
        std::shared_ptr<IConnectionManager> connectionManager_;
        bool listening_;
    };
}
