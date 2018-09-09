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
#include "StringHelpers.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <memory>

#include <boost/optional.hpp>

namespace Forum::Authorization
{
    typedef uint_fast16_t EnumIntType;

    //Changing existing enum members breaks backwards compatibility

    enum class DiscussionThreadMessagePrivilege : EnumIntType
    {
        VIEW,
        VIEW_REQUIRED_PRIVILEGES,
        VIEW_ASSIGNED_PRIVILEGES,
        VIEW_CREATOR_USER,
        VIEW_IP_ADDRESS,
        VIEW_VOTES,
        UP_VOTE,
        DOWN_VOTE,
        RESET_VOTE,
        ADD_COMMENT,
        SET_COMMENT_TO_SOLVED,
        GET_MESSAGE_COMMENTS,
        CHANGE_CONTENT,
        DELETE,
        MOVE,

        ADJUST_PRIVILEGE,

        COUNT
    };

    const StringView DiscussionThreadMessagePrivilegeStrings[] =
    {
        "view",
        "view_required_privileges",
        "view_assigned_privileges",
        "view_creator_user",
        "view_ip_address",
        "view_votes",
        "up_vote",
        "down_vote",
        "reset_vote",
        "add_comment",
        "set_comment_to_solved",
        "get_message_comments",
        "change_content",
        "delete",
        "move",
        "adjust_privilege"
    };

    const DiscussionThreadMessagePrivilege DiscussionThreadMessagePrivilegesToSerialize[] =
    {
        DiscussionThreadMessagePrivilege::VIEW_REQUIRED_PRIVILEGES,
        DiscussionThreadMessagePrivilege::VIEW_ASSIGNED_PRIVILEGES,
        DiscussionThreadMessagePrivilege::UP_VOTE,
        DiscussionThreadMessagePrivilege::DOWN_VOTE,
        DiscussionThreadMessagePrivilege::RESET_VOTE,
        DiscussionThreadMessagePrivilege::ADD_COMMENT,
        DiscussionThreadMessagePrivilege::SET_COMMENT_TO_SOLVED,
        DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS,
        DiscussionThreadMessagePrivilege::CHANGE_CONTENT,
        DiscussionThreadMessagePrivilege::DELETE,
        DiscussionThreadMessagePrivilege::MOVE,
        DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE
    };

    enum class DiscussionThreadPrivilege : EnumIntType
    {
        VIEW,
        VIEW_REQUIRED_PRIVILEGES,
        VIEW_ASSIGNED_PRIVILEGES,
        GET_SUBSCRIBED_USERS,
        SUBSCRIBE,
        UNSUBSCRIBE,
        ADD_MESSAGE,
        CHANGE_NAME,
        CHANGE_PIN_DISPLAY_ORDER,
        ADD_TAG,
        REMOVE_TAG,
        DELETE,
        MERGE,

        ADJUST_PRIVILEGE,

        COUNT
    };

    const StringView DiscussionThreadPrivilegeStrings[] =
    {
        "view",
        "view_required_privileges",
        "view_assigned_privileges",
        "get_subscribed_users",
        "subscribe",
        "unsubscribe",
        "add_message",
        "change_name",
        "change_pin_display_order",
        "add_tag",
        "remove_tag",
        "delete",
        "merge",
        "adjust_privilege"
    };

    const DiscussionThreadPrivilege DiscussionThreadPrivilegesToSerialize[] =
    {
        DiscussionThreadPrivilege::VIEW_REQUIRED_PRIVILEGES,
        DiscussionThreadPrivilege::VIEW_ASSIGNED_PRIVILEGES,
        DiscussionThreadPrivilege::GET_SUBSCRIBED_USERS,
        DiscussionThreadPrivilege::SUBSCRIBE,
        DiscussionThreadPrivilege::UNSUBSCRIBE,
        DiscussionThreadPrivilege::ADD_MESSAGE,
        DiscussionThreadPrivilege::CHANGE_NAME,
        DiscussionThreadPrivilege::CHANGE_PIN_DISPLAY_ORDER,
        DiscussionThreadPrivilege::ADD_TAG,
        DiscussionThreadPrivilege::REMOVE_TAG,
        DiscussionThreadPrivilege::DELETE,
        DiscussionThreadPrivilege::MERGE,
        DiscussionThreadPrivilege::ADJUST_PRIVILEGE
    };

    enum class DiscussionTagPrivilege : EnumIntType
    {
        VIEW,
        VIEW_REQUIRED_PRIVILEGES,
        VIEW_ASSIGNED_PRIVILEGES,
        GET_DISCUSSION_THREADS,
        CHANGE_NAME,
        CHANGE_UIBLOB,
        DELETE,
        MERGE,

        ADJUST_PRIVILEGE,

        COUNT
    };

    const StringView DiscussionTagPrivilegeStrings[] =
    {
        "view",
        "view_required_privileges",
        "view_assigned_privileges",
        "get_discussion_threads",
        "change_name",
        "change_uiblob",
        "delete",
        "merge",
        "adjust_privilege"
    };


    const DiscussionTagPrivilege DiscussionTagPrivilegesToSerialize[] =
    {
        DiscussionTagPrivilege::VIEW_REQUIRED_PRIVILEGES,
        DiscussionTagPrivilege::VIEW_ASSIGNED_PRIVILEGES,
        DiscussionTagPrivilege::CHANGE_NAME,
        DiscussionTagPrivilege::CHANGE_UIBLOB,
        DiscussionTagPrivilege::DELETE,
        DiscussionTagPrivilege::MERGE,
        DiscussionTagPrivilege::ADJUST_PRIVILEGE
    };

    enum class DiscussionCategoryPrivilege : EnumIntType
    {
        VIEW,
        VIEW_REQUIRED_PRIVILEGES,
        VIEW_ASSIGNED_PRIVILEGES,
        GET_DISCUSSION_THREADS,
        CHANGE_NAME,
        CHANGE_DESCRIPTION,
        CHANGE_PARENT,
        CHANGE_DISPLAYORDER,
        ADD_TAG,
        REMOVE_TAG,
        DELETE,

        ADJUST_PRIVILEGE,

        COUNT
    };

    const StringView DiscussionCategoryPrivilegeStrings[] =
    {
        "view",
        "view_required_privileges",
        "view_assigned_privileges",
        "get_discussion_threads",
        "change_name",
        "change_description",
        "change_parent",
        "change_displayorder",
        "add_tag",
        "remove_tag",
        "delete",
        "adjust_privilege"
    };

    const DiscussionCategoryPrivilege DiscussionCategoryPrivilegesToSerialize[] =
    {
        DiscussionCategoryPrivilege::VIEW_REQUIRED_PRIVILEGES,
        DiscussionCategoryPrivilege::VIEW_ASSIGNED_PRIVILEGES,
        DiscussionCategoryPrivilege::CHANGE_NAME,
        DiscussionCategoryPrivilege::CHANGE_DESCRIPTION,
        DiscussionCategoryPrivilege::CHANGE_PARENT,
        DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER,
        DiscussionCategoryPrivilege::ADD_TAG,
        DiscussionCategoryPrivilege::REMOVE_TAG,
        DiscussionCategoryPrivilege::DELETE,
        DiscussionCategoryPrivilege::ADJUST_PRIVILEGE,
    };

    enum class ForumWidePrivilege : EnumIntType
    {
        ADD_USER,
        GET_ENTITIES_COUNT,
        GET_VERSION,
        GET_ALL_USERS,
        GET_USER_INFO,
        GET_DISCUSSION_THREADS_OF_USER,
        GET_DISCUSSION_THREAD_MESSAGES_OF_USER,
        GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER,
        GET_ALL_DISCUSSION_CATEGORIES,
        GET_DISCUSSION_CATEGORIES_FROM_ROOT,
        GET_ALL_DISCUSSION_TAGS,
        GET_ALL_DISCUSSION_THREADS,
        GET_ALL_MESSAGE_COMMENTS,
        GET_MESSAGE_COMMENTS_OF_USER,
        ADD_DISCUSSION_CATEGORY,
        ADD_DISCUSSION_TAG,
        ADD_DISCUSSION_THREAD,
        CHANGE_OWN_USER_NAME,
        CHANGE_OWN_USER_INFO,
        CHANGE_ANY_USER_NAME,
        CHANGE_ANY_USER_INFO,
        DELETE_ANY_USER,

        VIEW_FORUM_WIDE_REQUIRED_PRIVILEGES,
        VIEW_FORUM_WIDE_ASSIGNED_PRIVILEGES,
        VIEW_USER_ASSIGNED_PRIVILEGES,
        ADJUST_FORUM_WIDE_PRIVILEGE,

        CHANGE_OWN_USER_TITLE,
        CHANGE_ANY_USER_TITLE,
        CHANGE_OWN_USER_SIGNATURE,
        CHANGE_ANY_USER_SIGNATURE,
        CHANGE_OWN_USER_LOGO,
        CHANGE_ANY_USER_LOGO,
        DELETE_OWN_USER_LOGO,
        DELETE_ANY_USER_LOGO,
        
        NO_THROTTLING,

        COUNT
    };

    const StringView ForumWidePrivilegeStrings[] =
    {
        "add_user",
        "get_entities_count",
        "get_version",
        "get_all_users",
        "get_user_info",
        "get_discussion_threads_of_user",
        "get_discussion_thread_messages_of_user",
        "get_subscribed_discussion_threads_of_user",
        "get_all_discussion_categories",
        "get_discussion_categories_from_root",
        "get_all_discussion_tags",
        "get_all_discussion_threads",
        "get_all_message_comments",
        "get_message_comments_of_user",
        "add_discussion_category",
        "add_discussion_tag",
        "add_discussion_thread",
        "change_own_user_name",
        "change_own_user_info",
        "change_any_user_name",
        "change_any_user_info",
        "delete_any_user",

        "view_forum_wide_required_privileges",
        "view_forum_wide_assigned_privileges",
        "view_user_assigned_privileges",
        "adjust_forum_wide_privilege",

        "change_own_user_title",
        "change_any_user_title",
        "change_own_user_signature",
        "change_any_user_signature",
        "change_own_user_logo",
        "change_any_user_logo",
        "delete_own_user_logo",
        "delete_any_user_logo",

        "no_throttling",
    };

    const ForumWidePrivilege ForumWidePrivilegesToSerialize[] =
    {
        ForumWidePrivilege::GET_ENTITIES_COUNT,
        ForumWidePrivilege::GET_VERSION,
        ForumWidePrivilege::GET_ALL_USERS,
        ForumWidePrivilege::GET_USER_INFO,
        ForumWidePrivilege::GET_DISCUSSION_THREADS_OF_USER,
        ForumWidePrivilege::GET_DISCUSSION_THREAD_MESSAGES_OF_USER,
        ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER,
        ForumWidePrivilege::GET_ALL_DISCUSSION_CATEGORIES,
        ForumWidePrivilege::GET_DISCUSSION_CATEGORIES_FROM_ROOT,
        ForumWidePrivilege::GET_ALL_DISCUSSION_TAGS,
        ForumWidePrivilege::GET_ALL_DISCUSSION_THREADS,
        ForumWidePrivilege::GET_ALL_MESSAGE_COMMENTS,
        ForumWidePrivilege::GET_MESSAGE_COMMENTS_OF_USER,
        ForumWidePrivilege::ADD_DISCUSSION_CATEGORY,
        ForumWidePrivilege::ADD_DISCUSSION_TAG,
        ForumWidePrivilege::ADD_DISCUSSION_THREAD,
        ForumWidePrivilege::CHANGE_OWN_USER_NAME,
        ForumWidePrivilege::CHANGE_OWN_USER_INFO,
        ForumWidePrivilege::CHANGE_ANY_USER_NAME,
        ForumWidePrivilege::CHANGE_ANY_USER_INFO,
        ForumWidePrivilege::DELETE_ANY_USER,
        ForumWidePrivilege::VIEW_FORUM_WIDE_REQUIRED_PRIVILEGES,
        ForumWidePrivilege::VIEW_FORUM_WIDE_ASSIGNED_PRIVILEGES,
        ForumWidePrivilege::VIEW_USER_ASSIGNED_PRIVILEGES,
        ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE,
        ForumWidePrivilege::CHANGE_OWN_USER_TITLE,
        ForumWidePrivilege::CHANGE_ANY_USER_TITLE,
        ForumWidePrivilege::CHANGE_OWN_USER_SIGNATURE,
        ForumWidePrivilege::CHANGE_ANY_USER_SIGNATURE,
        ForumWidePrivilege::CHANGE_OWN_USER_LOGO,
        ForumWidePrivilege::CHANGE_ANY_USER_LOGO,
        ForumWidePrivilege::DELETE_OWN_USER_LOGO,
        ForumWidePrivilege::DELETE_ANY_USER_LOGO,
        ForumWidePrivilege::NO_THROTTLING
    };

    enum class ForumWideDefaultPrivilegeDuration : EnumIntType
    {
        CREATE_DISCUSSION_THREAD,
        CREATE_DISCUSSION_THREAD_MESSAGE,

        COUNT
    };

    const StringView ForumWideDefaultPrivilegeDurationStrings[] =
    {
        "create_discussion_thread",
        "create_discussion_thread_message"
    };

    enum class UserActionThrottling : EnumIntType
    {
        NEW_CONTENT,
        EDIT_CONTENT,
        EDIT_PRIVILEGES,
        VOTE,
        SUBSCRIBE,

        COUNT
    };

    typedef int16_t PrivilegeValueIntType;
    typedef boost::optional<PrivilegeValueIntType> PrivilegeValueType;
    typedef int_fast64_t PrivilegeDurationIntType;

    const std::pair<uint16_t, PrivilegeDurationIntType> ThrottlingDefaultValues[] =
    {
        {10,  600}, //max 10 new content actions every 10 minuts
        {10,  300}, //max 10 edits of content every 5 minutes
        {10,   10}, //max 10 edits of privileges every 10 seconds
        {10, 3600}, //max 10 votes every hour
        {10,  600}  //max 10 subscriptions every 10 minutes
    };

    struct PrivilegeDefaultLevel
    {
        PrivilegeValueIntType value;
        PrivilegeDurationIntType duration;
    };

    typedef boost::optional<PrivilegeDefaultLevel> PrivilegeDefaultLevelType;

    static constexpr PrivilegeValueIntType MinPrivilegeValue = -32000;
    static constexpr PrivilegeValueIntType MaxPrivilegeValue = 32000;
    static constexpr PrivilegeDurationIntType UnlimitedDuration = 0;

    template<typename T>
    T optionalOrZero(boost::optional<T> value)
    {
        return value ? *value : 0;
    }

    inline PrivilegeValueType minimumPrivilegeValue(PrivilegeValueIntType first, PrivilegeValueIntType second)
    {
        return std::min(first, second);
    }

    inline PrivilegeValueType minimumPrivilegeValue(PrivilegeValueType first, PrivilegeValueType second)
    {
        if ( ! first) return second;
        if ( ! second) return first;

        return minimumPrivilegeValue(*first, *second);
    }

    inline PrivilegeValueType minimumPrivilegeValue(PrivilegeValueIntType first, PrivilegeValueType second)
    {
        if ( ! second) return first;

        return minimumPrivilegeValue(first, *second);
    }

    inline PrivilegeValueType minimumPrivilegeValue(PrivilegeValueType first, PrivilegeValueIntType second)
    {
        if ( ! first) return second;

        return minimumPrivilegeValue(*first, second);
    }

    inline PrivilegeValueType maximumPrivilegeValue(PrivilegeValueIntType first, PrivilegeValueIntType second)
    {
        return std::max(first, second);
    }

    inline PrivilegeValueType maximumPrivilegeValue(PrivilegeValueType first, PrivilegeValueType second)
    {
        if ( ! first) return second;
        if ( ! second) return first;

        return maximumPrivilegeValue(*first, *second);
    }

    inline PrivilegeValueType maximumPrivilegeValue(PrivilegeValueIntType first, PrivilegeValueType second)
    {
        if ( ! second) return first;

        return maximumPrivilegeValue(first, *second);
    }

    inline PrivilegeValueType maximumPrivilegeValue(PrivilegeValueType first, PrivilegeValueIntType second)
    {
        if ( ! first) return second;

        return maximumPrivilegeValue(*first, second);
    }

    inline PrivilegeDurationIntType calculatePrivilegeExpires(const PrivilegeDurationIntType start,
                                                              PrivilegeDurationIntType duration)
    {
        static_assert(sizeof(PrivilegeDurationIntType) >= 8, "PrivilegeDurationIntType should be at least 64-bit wide");

        if (duration == UnlimitedDuration)
        {
            return 0;
        }

        constexpr auto maxHalf = std::numeric_limits<PrivilegeDurationIntType>::max() / 2;
        assert(start >= 0);
        assert(start < maxHalf);

        if (duration >= maxHalf)
        {
            duration -= start;
        }
        return start + duration;
    }

    struct DiscussionThreadMessagePrivilegeStore
    {
        DECLARE_ABSTRACT_MANDATORY(DiscussionThreadMessagePrivilegeStore)

        void setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege,
                                                 PrivilegeValueIntType value)
        {
            if (privilege < DiscussionThreadMessagePrivilege::COUNT)
            {
                if ( ! discussionThreadMessagePrivileges_)
                {
                    discussionThreadMessagePrivileges_.reset(
                            new PrivilegeValueType[static_cast<EnumIntType>(DiscussionThreadMessagePrivilege::COUNT)]);
                }
                discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)] = value;
            }
        }

        virtual PrivilegeValueType getDiscussionThreadMessagePrivilege(
                DiscussionThreadMessagePrivilege privilege) const
        {
            if ((privilege < DiscussionThreadMessagePrivilege::COUNT) && discussionThreadMessagePrivileges_)
            {
                return discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)];
            }
            return{};
        }

    private:
        std::unique_ptr<PrivilegeValueType[]> discussionThreadMessagePrivileges_;
    };

    struct DiscussionThreadPrivilegeStore : DiscussionThreadMessagePrivilegeStore
    {
        DECLARE_ABSTRACT_MANDATORY(DiscussionThreadPrivilegeStore)

        void setDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege,
                                          PrivilegeValueIntType value)
        {
            if (privilege < DiscussionThreadPrivilege::COUNT)
            {
                discussionThreadPrivileges_[static_cast<EnumIntType>(privilege)] = value;
            }
        }

        virtual PrivilegeValueType getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
        {
            if (privilege < DiscussionThreadPrivilege::COUNT)
            {
                return discussionThreadPrivileges_[static_cast<EnumIntType>(privilege)];
            }
            return{};
        }

    private:
        PrivilegeValueType discussionThreadPrivileges_
            [static_cast<EnumIntType>(DiscussionThreadPrivilege::COUNT)];
    };

    struct DiscussionTagPrivilegeStore : DiscussionThreadPrivilegeStore
    {
        DECLARE_ABSTRACT_MANDATORY(DiscussionTagPrivilegeStore)

        void setDiscussionTagPrivilege(DiscussionTagPrivilege privilege, PrivilegeValueIntType value)
        {
            if (privilege < DiscussionTagPrivilege::COUNT)
            {
                discussionTagPrivileges_[static_cast<EnumIntType>(privilege)] = value;
            }
        }

        virtual PrivilegeValueType getDiscussionTagPrivilege(DiscussionTagPrivilege privilege) const
        {
            if (privilege < DiscussionTagPrivilege::COUNT)
            {
                return discussionTagPrivileges_[static_cast<EnumIntType>(privilege)];
            }
            return{};
        }

    private:
        PrivilegeValueType discussionTagPrivileges_
            [static_cast<EnumIntType>(DiscussionTagPrivilege::COUNT)];
    };

    struct DiscussionCategoryPrivilegeStore
    {
        DECLARE_ABSTRACT_MANDATORY(DiscussionCategoryPrivilegeStore)

        void setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value)
        {
            if (privilege < DiscussionCategoryPrivilege::COUNT)
            {
                discussionCategoryPrivileges_[static_cast<EnumIntType>(privilege)] = value;
            }
        }

        virtual PrivilegeValueType getDiscussionCategoryPrivilege(DiscussionCategoryPrivilege privilege) const
        {
            if (privilege < DiscussionCategoryPrivilege::COUNT)
            {
                return discussionCategoryPrivileges_[static_cast<EnumIntType>(privilege)];
            }
            return{};
        }

    private:
        PrivilegeValueType discussionCategoryPrivileges_
            [static_cast<EnumIntType>(DiscussionCategoryPrivilege::COUNT)];
    };

    struct ForumWidePrivilegeStore : DiscussionTagPrivilegeStore, DiscussionCategoryPrivilegeStore
    {
        DECLARE_ABSTRACT_MANDATORY(ForumWidePrivilegeStore)

        void setForumWidePrivilege(ForumWidePrivilege privilege, PrivilegeValueIntType value)
        {
            if (privilege < ForumWidePrivilege::COUNT)
            {
                forumWidePrivileges_[static_cast<EnumIntType>(privilege)] = value;
            }
        }

        virtual PrivilegeValueType getForumWidePrivilege(ForumWidePrivilege privilege) const
        {
            if (privilege < ForumWidePrivilege::COUNT)
            {
                return forumWidePrivileges_[static_cast<EnumIntType>(privilege)];
            }
            return{};
        }

        void setForumWideDefaultPrivilegeLevel(ForumWideDefaultPrivilegeDuration privilege,
                                               PrivilegeDefaultLevel value)
        {
            if (privilege < ForumWideDefaultPrivilegeDuration::COUNT)
            {
                forumWideDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)] = value;
            }
        }

        virtual PrivilegeDefaultLevelType getForumWideDefaultPrivilegeLevel(
                ForumWideDefaultPrivilegeDuration privilege) const
        {
            if (privilege < ForumWideDefaultPrivilegeDuration::COUNT)
            {
                return forumWideDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)];
            }
            return{};
        }

    private:
        PrivilegeValueType forumWidePrivileges_[static_cast<EnumIntType>(ForumWidePrivilege::COUNT)];
        PrivilegeDefaultLevelType forumWideDefaultPrivilegeDuration_
            [static_cast<EnumIntType>(ForumWideDefaultPrivilegeDuration::COUNT)];
    };
}
