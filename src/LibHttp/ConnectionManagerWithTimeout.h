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
#include "TimeoutManager.h"

#include <cstdint>
#include <memory>

namespace Http
{
    class ConnectionManagerWithTimeout : public IConnectionManager
    {
    public:
        explicit ConnectionManagerWithTimeout(boost::asio::io_service& ioService, 
            std::shared_ptr<IConnectionManager> delegateTo, size_t connectionTimeoutSeconds);

        ConnectionIdentifier newConnection(IConnectionManager* manager, boost::asio::ip::tcp::socket&& socket) override;
        void closeConnection(ConnectionIdentifier identifier) override;
        void disconnectConnection(ConnectionIdentifier identifier) override;

        void stop() override;

    private:
        void startTimer();
        void onCheckTimeout(const boost::system::error_code& ec);

        std::shared_ptr<IConnectionManager> delegateTo_;
        boost::asio::deadline_timer timeoutTimer_;
        std::atomic<std::int64_t> nrOfCurrentlyOpenConnections_{ 0 };
        TimeoutManager<ConnectionIdentifier> timeoutManager_;
    };
}
