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

#include "ConnectionManagerWithTimeout.h"

using namespace Http;

constexpr static int CheckTimeoutEverySeconds = 1;

ConnectionManagerWithTimeout::ConnectionManagerWithTimeout(boost::asio::io_service& ioService, 
    std::shared_ptr<IConnectionManager> delegateTo, const size_t connectionTimeoutSeconds) :
    delegateTo_{ std::move(delegateTo) }, 
    timeoutTimer_{ ioService, boost::posix_time::seconds(CheckTimeoutEverySeconds) },
    timeoutManager_
    {
            [this](auto identifier) { this->disconnectConnection(identifier); },
            static_cast<decltype(timeoutManager_)::Timestamp>(connectionTimeoutSeconds)
    }
{
    startTimer();
}

IConnectionManager::ConnectionIdentifier ConnectionManagerWithTimeout::newConnection(IConnectionManager* manager,
    boost::asio::ip::tcp::socket&& socket)
{
    const auto result = delegateTo_->newConnection(manager ? manager : this, std::move(socket));

    if (result)
    {
        nrOfCurrentlyOpenConnections_ += 1;
        timeoutManager_.addExpireIn(result, timeoutManager_.defaultTimeout());
    }
    return result;
}

void ConnectionManagerWithTimeout::closeConnection(const ConnectionIdentifier identifier)
{
    if (identifier)
    {
        timeoutManager_.remove(identifier);
    }

    delegateTo_->closeConnection(identifier);
    nrOfCurrentlyOpenConnections_ -= 1;
}

void ConnectionManagerWithTimeout::disconnectConnection(const ConnectionIdentifier identifier)
{
    delegateTo_->disconnectConnection(identifier);
}

void ConnectionManagerWithTimeout::stop()
{
    boost::asio::dispatch(timeoutTimer_.get_executor(), [&]
    {
        boost::system::error_code ec;

        timeoutTimer_.cancel(ec);
    });
    delegateTo_->stop();
}

void ConnectionManagerWithTimeout::startTimer()
{
    timeoutTimer_.async_wait([this](const boost::system::error_code& ec) { this->onCheckTimeout(ec); });
}

void ConnectionManagerWithTimeout::onCheckTimeout(const boost::system::error_code&)
{
    if (nrOfCurrentlyOpenConnections_ > 0)
    {
        timeoutManager_.checkTimeout();
    }
    timeoutTimer_.expires_at(timeoutTimer_.expires_at() + boost::posix_time::seconds(CheckTimeoutEverySeconds));

    startTimer();
}
