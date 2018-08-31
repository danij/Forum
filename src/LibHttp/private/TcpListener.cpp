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

#include "TcpListener.h"

using namespace Http;

TcpListener::TcpListener(boost::asio::io_service& ioService, std::string_view listenIpAddress,
    const uint16_t listenPort, std::shared_ptr<IConnectionManager> connectionManager) :

    listenIpAddress_{ boost::asio::ip::address::from_string(std::string{listenIpAddress}) },
    listenPort_{ listenPort }, acceptor_{ ioService }, currentSocket_{ ioService },
    connectionManager_{ std::move(connectionManager) }, listening_{ false }
{}

TcpListener::~TcpListener()
{
    if (listening_)
    {
        stopListening();
    }
}

void TcpListener::startListening()
{
    if (listening_) return;
    listening_ = true;

    boost::asio::ip::tcp::endpoint endpoint
    {
        listenIpAddress_,
        listenPort_
    };

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    startAccept();
}

void TcpListener::stopListening()
{
    if ( ! listening_) return;
    listening_ = false;

    acceptor_.get_io_service().dispatch([this]
    {
        boost::system::error_code ec;
        this->acceptor_.close(ec);
    });

    connectionManager_->stop();
}

void TcpListener::startAccept()
{
    acceptor_.async_accept(currentSocket_, [this](const boost::system::error_code& ec) { this->onAccept(ec); });
}

void TcpListener::onAccept(const boost::system::error_code& ec)
{
    if ( ! acceptor_.is_open())
    {
        return;
    }
    if (ec)
    {
        //TODO: handle error
        return;
    }
    connectionManager_->newConnection(nullptr, std::move(currentSocket_));

    startAccept();
}
