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

#include "StreamingConnection.h"

using namespace Http;

StreamingConnection::StreamingConnection(IConnectionManager& connectionManager, boost::asio::ip::tcp::socket&& socket, 
                                         boost::asio::io_context& context)
    : socket_{std::move(socket)}, strand_{context}, connectionManager_(connectionManager)
{}

void StreamingConnection::release()
{
    connectionManager_.closeConnection(this);
}

void StreamingConnection::startReading()
{
    boost::asio::post(strand_, [this]()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(readBuffer_), boost::asio::transfer_at_least(1),
            strand_.wrap(
                [this](const boost::system::error_code& ec, const size_t bytesTransferred)
                {
                    this->onRead(ec, bytesTransferred);
                }));
    });
}

void StreamingConnection::disconnect()
{
    closeSocket(socket_);
}

void StreamingConnection::onRead(const boost::system::error_code& ec, const size_t bytesTransferred)
{
    if (ec == boost::asio::error::eof)
    {
        //connection closed, nothing more to do regarding this connection
        release();
        return;
    }
    if (ec)
    {
        auto msg = ec.message();
        //TODO: handle error
        release();
        return;
    }
    if (0 == bytesTransferred)
    {
        release();
        return;
    }

    if (onBytesRead(readBuffer_.data(), bytesTransferred))
    {
        startReading();
    }
}

void StreamingConnection::onWritten(const boost::system::error_code& ec, const size_t bytesTransferred)
{
    if (ec)
    {
        //connection closed or error occured, nothing more to do regarding this connection
        release();
        return;
    }

    onWritten(bytesTransferred);
}

void Http::closeSocket(boost::asio::ip::tcp::socket& socket)
{
    try
    {
        socket.shutdown(boost::asio::socket_base::shutdown_both);
        socket.close();
    }
    catch (...)
    {
    }
}
