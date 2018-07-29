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

#include "EntityCollection.h"
#include "Observers.h"
#include "TypeHelpers.h"

#include <boost/dll.hpp>
#include <boost/property_tree/ptree.hpp>

#include <memory>

namespace Forum::Extensibility
{
    class IPlugin
    {
    public:
        DECLARE_INTERFACE_MANDATORY_NO_COPY(IPlugin)

        virtual StringView name() const noexcept = 0;
        virtual StringView version() const noexcept = 0;

        virtual void stop() = 0;
    };

    using PluginPtr = std::shared_ptr<IPlugin>;

    struct LoadedPlugin
    {
        boost::dll::shared_library library;
        PluginPtr plugin;
    };

    struct PluginInput
    {
        Entities::EntityCollection* globalEntityCollection;
        Repository::ReadEvents* readEvents;
        Repository::WriteEvents* writeEvents;
        const boost::property_tree::ptree* configuration;
    };

    using PluginLoaderFn = void __cdecl(PluginInput* input, PluginPtr* output);
}