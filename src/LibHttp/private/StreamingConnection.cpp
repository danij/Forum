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

StreamingConnection::StreamingConnection(IConnectionManager& connectionManager, boost::asio::ip::tcp::socket&& socket)
    : socket_{std::move(socket)}, strand_{socket_.get_io_context()}, connectionManager_(connectionManager)
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
                [this](const boost::system::error_code& ec, const size_t bytesTransfered)
                {
                    this->onRead(ec, bytesTransfered);
                }));
    });
}

void StreamingConnection::onRead(const boost::system::error_code& ec, const size_t bytesTransfered)
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
    if (0 == bytesTransfered)
    {
        release();
        return;
    }

    if (onBytesRead(readBuffer_.data(), bytesTransfered))
    {
        startReading();
    }
}

void StreamingConnection::onWritten(const boost::system::error_code& ec, size_t bytesTransfered)
{
    if (ec)
    {
        //connection closed or error occured, nothing more to do regarding this connection
        release();
        return;
    }

    onWritten(bytesTransfered);
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
