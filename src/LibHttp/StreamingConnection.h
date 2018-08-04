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

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

namespace Http
{
    class StreamingConnection : boost::noncopyable
    {
    public:
        StreamingConnection(IConnectionManager& connectionManager, boost::asio::ip::tcp::socket&& socket);
        virtual ~StreamingConnection() = default;

        void startReading();

    protected:
        void release();
        virtual bool onBytesRead(char* bytes, size_t bytesTransfered) = 0;
        virtual void onWritten(size_t bytesTransfered) = 0;

        template<typename ConstBuffer>
        void write(const ConstBuffer& buffer)
        {
            strand_.post([this, &buffer]()
            {
                boost::asio::async_write(socket_, buffer, boost::asio::transfer_all(), 
                    strand_.wrap([this](const boost::system::error_code& ec, const size_t bytesTransfered)
                    {
                        this->onWritten(ec, bytesTransfered);
                    }));
            });
        }     

        boost::asio::ip::tcp::socket socket_;
        std::array<char, 1024> readBuffer_{};

    private:
        void onRead(const boost::system::error_code& ec, size_t bytesTransfered);
        void onWritten(const boost::system::error_code& ec, size_t bytesTransfered);

        boost::asio::io_service::strand strand_;
        IConnectionManager& connectionManager_;
    };

    void closeSocket(boost::asio::ip::tcp::socket& socket);
}
