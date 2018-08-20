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

#include "TcpListener.h"
#include "HttpRouter.h"
#include "CommandHandler.h"
#include "MemoryRepositoryCommon.h"
#include "ServiceEndpointManager.h"
#include "EventObserver.h"
#include "Plugin.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Forum
{
    class Application final : boost::noncopyable
    {
    public:
        int run(int argc, const char* argv[]);

    private:
        void cleanup();
        bool initialize();
        bool loadConfiguration(const std::string& fileName);
        void validateConfiguration();
        bool createCommandHandler();
        bool importEvents();
        bool initializeHttp();
        bool initializeLogging();
        bool loadPlugins();
        void prepareToStop();

        std::unique_ptr<Http::TcpListener> tcpListener_;
        std::unique_ptr<Http::TcpListener> tcpListenerAuth_;

        std::unique_ptr<Commands::CommandHandler> commandHandler_;
        std::unique_ptr<Commands::ServiceEndpointManager> endpointManager_;
        std::unique_ptr<Persistence::EventObserver> persistenceObserver_;

        Repository::MemoryStoreRef memoryStore_;
        Entities::EntityCollectionRef entityCollection_;
        Repository::DirectWriteRepositoryCollection directWriteRepositories_;

        std::vector<Extensibility::LoadedPlugin> plugins_;
    };
}
