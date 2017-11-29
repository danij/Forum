/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "Configuration.h"
#include "ContextProviderMocks.h"
#include "Repository.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/noncopyable.hpp>

#include <string>

namespace Forum
{
    namespace Helpers
    {
        inline void assertStatusCodeEqual(Repository::StatusCode expected, Repository::StatusCode actual)
        {
            BOOST_REQUIRE_EQUAL(expected, actual);
        }

        template <typename TStatusObj>
        void assertStatusCodeEqual(Repository::StatusCode expected, const TStatusObj& obj)
        {
            assertStatusCodeEqual(expected, Repository::StatusCode(obj.template get<uint32_t>("status")));
        }

        inline bool isIdEmpty(const std::string& id)
        {
            return Entities::UuidString(id) == Entities::UuidString::empty;
        }

        template<size_t StackSize>
        bool operator==(const JsonReadyStringWithSortKey<StackSize>& first, const std::string& second)
        {
            return first == JsonReadyStringWithSortKey<StackSize>(StringView(second));
        }

        extern const std::string sampleValidIdString;
        extern const Entities::IdType sampleValidId;
        extern const std::string sampleValidIdString2;
        extern const Entities::IdType sampleValidId2;
        extern const std::string sampleMessageContent;

        bool treeContains(const boost::property_tree::ptree& tree, const std::string& key);

        /**
         * Provides a means to execute code at the end of a test case even if it fails or an exception is thrown
         */
        template <typename TAction>
        struct Disposer final
        {
            explicit Disposer (TAction action) : action_(action) {}
            ~Disposer() { action_(); }

        private:
            TAction action_;
        };

        template <typename TAction>
        auto createDisposer(TAction action)
        {
            return Disposer<TAction>(action);
        }

        struct ConfigChanger final : private boost::noncopyable
        {
            explicit ConfigChanger(std::function<void(Configuration::Config&)>&& configChangeAction)
            {
                oldConfig_ = *Configuration::getGlobalConfig();

                Configuration::Config newConfig(oldConfig_);
                configChangeAction(newConfig);
                setGlobalConfig(newConfig);
            }
            ~ConfigChanger()
            {
                setGlobalConfig(oldConfig_);
            }
        private:
            Configuration::Config oldConfig_;
        };

        struct TimestampChanger final : private boost::noncopyable
        {
            explicit TimestampChanger(Entities::Timestamp value)
            {
                Context::setCurrentTimeMockForCurrentThread([=]() { return value; });
            }
            ~TimestampChanger()
            {
                Context::resetCurrentTimeMock();
            }
        };

        struct LoggedInUserChanger final : private boost::noncopyable
        {
            explicit LoggedInUserChanger(Entities::IdType userId)
            {
                oldId_ = Context::getCurrentUserId();
                Context::setCurrentUserId(userId);
            }
            ~LoggedInUserChanger()
            {
                Context::setCurrentUserId(oldId_);
            }

        private:
            Entities::IdType oldId_;
        };

        struct IpChanger final : private boost::noncopyable
        {
            IpChanger(const std::string& newIp)
            {
                oldIp_ = Context::getCurrentUserIpAddress();
                Context::setCurrentUserIpAddress(newIp.c_str());
            }
            ~IpChanger()
            {
                Context::setCurrentUserIpAddress(oldIp_);
            }

        private:
            IpAddress oldIp_;
        };
    }
}
