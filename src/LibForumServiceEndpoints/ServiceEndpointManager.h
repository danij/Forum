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

#include "CommandHandler.h"
#include "HttpRouter.h"

#include <boost/noncopyable.hpp>

namespace Forum::Commands
{
    class ServiceEndpointManager final : boost::noncopyable
    {
    public:
        explicit ServiceEndpointManager(CommandHandler& handler);
        ~ServiceEndpointManager();

        void registerRoutes(Http::HttpRouter& router);
        void registerAuthRoutes(Http::HttpRouter& router);

    private:
        struct ServiceEndpointManagerImpl;
        ServiceEndpointManagerImpl* impl_;
    };
}
