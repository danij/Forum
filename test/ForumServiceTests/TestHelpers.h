#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/test/unit_test.hpp>

#include "Configuration.h"
#include "ContextProviderMocks.h"
#include "Repository.h"

namespace Forum
{
    namespace Helpers
    {
        inline void assertStatusCodeEqual(Forum::Repository::StatusCode expected, Forum::Repository::StatusCode actual)
        {
            BOOST_REQUIRE_EQUAL(expected, actual);
        }

        template <typename TStatusObj>
        inline void assertStatusCodeEqual(Forum::Repository::StatusCode expected, const TStatusObj& obj)
        {
            assertStatusCodeEqual(expected, Forum::Repository::StatusCode(obj.template get<uint32_t>("status")));
        }

        inline bool isIdEmpty(const std::string& id)
        {
            return Forum::Entities::UuidString(id) == Forum::Entities::UuidString::empty;
        }

        extern Forum::Entities::IdType sampleValidId;

        bool treeContains(const boost::property_tree::ptree& tree, const std::string& key);

        /**
         * Provides a means to execute code at the end of a test case even if it fails or an exception is thrown
         */
        template <typename TAction>
        struct Disposer final
        {
            Disposer (TAction action) : action_(action) {}
            ~Disposer() { action_(); }

        private:
            TAction action_;
        };

        template <typename TAction>
        inline auto createDisposer(TAction action)
        {
            return Disposer<TAction>(action);
        }

        struct ConfigChanger final
        {
            ConfigChanger(std::function<void(Forum::Configuration::Config&)> configChangeAction)
            {
                oldConfig_ = *Forum::Configuration::getGlobalConfig();

                Forum::Configuration::Config newConfig(oldConfig_);
                configChangeAction(newConfig);
                Forum::Configuration::setGlobalConfig(newConfig);
            }
            ~ConfigChanger()
            {
                Forum::Configuration::setGlobalConfig(oldConfig_);
            }
        private:
            Forum::Configuration::Config oldConfig_;
        };

        struct TimestampChanger final
        {
            TimestampChanger(Forum::Entities::Timestamp value)
            {
                Forum::Context::setCurrentTimeMockForCurrentThread([=]() { return value; });
            }
            ~TimestampChanger()
            {
                Forum::Context::resetCurrentTimeMock();
            }
        };

        struct LoggedInUserChanger final
        {
            LoggedInUserChanger(Forum::Entities::IdType userId)
            {
                oldId_ = Forum::Context::getCurrentUserId();
                Forum::Context::setCurrentUserId(userId);
            }
            ~LoggedInUserChanger()
            {
                Forum::Context::setCurrentUserId(oldId_);
            }

        private:
            Forum::Entities::IdType oldId_;
        };
    }
}
