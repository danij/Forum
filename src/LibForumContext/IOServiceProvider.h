/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "TypeHelpers.h"

#include <boost/asio/io_context.hpp>

namespace Forum::Network
{
    class IIOServiceProvider
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IIOServiceProvider)

        virtual boost::asio::io_context& getIOService() = 0;
        virtual void start() = 0;
        virtual void waitForStop() = 0;
        virtual void stop() = 0;
    };
}
