#pragma once

#include "TypeHelpers.h"
#include "StringHelpers.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>

#include <boost/optional.hpp>

namespace Forum
{
    namespace Authorization
    {
        typedef uint_fast32_t EnumIntType;

        enum class DiscussionThreadMessagePrivilege : EnumIntType
        {
            VIEW,
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

        const std::pair<DiscussionThreadMessagePrivilege, StringView> DiscussionThreadMessagePrivilegesToSerialize[] =
        {
            { DiscussionThreadMessagePrivilege::UP_VOTE, "up_vote" },
            { DiscussionThreadMessagePrivilege::DOWN_VOTE, "down_vote" },
            { DiscussionThreadMessagePrivilege::CHANGE_CONTENT, "change_content" },
            { DiscussionThreadMessagePrivilege::DELETE, "delete" }
        };

        enum class DiscussionThreadMessageDefaultPrivilegeDuration : EnumIntType
        {
            RESET_VOTE,
            CHANGE_CONTENT,
            DELETE,

            COUNT
        };

        const StringView DiscussionThreadMessageDefaultPrivilegeDurationStrings[] =
        {
            "reset_vote",
            "change_content",
            "delete"
        };

        enum class DiscussionThreadPrivilege : EnumIntType
        {
            VIEW,
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

        const std::pair<DiscussionThreadPrivilege, StringView> DiscussionThreadPrivilegesToSerialize[] =
        {
            { DiscussionThreadPrivilege::ADD_MESSAGE, "add_message" },
            { DiscussionThreadPrivilege::CHANGE_NAME, "change_name" },
            { DiscussionThreadPrivilege::CHANGE_PIN_DISPLAY_ORDER, "change_pin_display_order" },
            { DiscussionThreadPrivilege::ADD_TAG, "add_tag" },
            { DiscussionThreadPrivilege::REMOVE_TAG, "remove_tag" },
            { DiscussionThreadPrivilege::DELETE, "delete" },
        };

        enum class DiscussionTagPrivilege : EnumIntType
        {
            VIEW,
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
            "get_discussion_threads",
            "change_name",
            "change_uiblob",
            "delete",
            "merge",
            "adjust_privilege"
        };


        const std::pair<DiscussionTagPrivilege, StringView> DiscussionTagPrivilegesToSerialize[] =
        {
            { DiscussionTagPrivilege::CHANGE_NAME, "change_name" },
            { DiscussionTagPrivilege::CHANGE_UIBLOB, "change_uiblob" },
            { DiscussionTagPrivilege::DELETE, "delete" }
        };

        enum class DiscussionCategoryPrivilege : EnumIntType
        {
            VIEW,
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

        const std::pair<DiscussionCategoryPrivilege, StringView> DiscussionCategoryPrivilegesToSerialize[] =
        {
            { DiscussionCategoryPrivilege::CHANGE_NAME, "change_name" },
            { DiscussionCategoryPrivilege::CHANGE_DESCRIPTION, "change_description" },
            { DiscussionCategoryPrivilege::CHANGE_PARENT, "change_parent" },
            { DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER, "change_displayorder" },
            { DiscussionCategoryPrivilege::ADD_TAG, "add_tag" },
            { DiscussionCategoryPrivilege::REMOVE_TAG, "remove_tag" },
            { DiscussionCategoryPrivilege::DELETE, "delete" }
        };

        enum class ForumWidePrivilege : EnumIntType
        {
            ADD_USER,
            LOGIN,
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

            ADJUST_FORUM_WIDE_PRIVILEGE,

            COUNT
        };

        const StringView ForumWidePrivilegeStrings[] =
        {
            "add_user",
            "login",
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
            "adjust_forum_wide_privilege"
        };

        const std::pair<ForumWidePrivilege, StringView> ForumWidePrivilegesToSerialize[] =
        {
            { ForumWidePrivilege::GET_ENTITIES_COUNT, "get_entities_count" },
            { ForumWidePrivilege::GET_VERSION, "get_version" },
            { ForumWidePrivilege::GET_ALL_USERS, "get_all_users" },
            { ForumWidePrivilege::GET_USER_INFO, "get_user_info" },
            { ForumWidePrivilege::GET_DISCUSSION_THREADS_OF_USER, "get_discussion_threads_of_user" },
            { ForumWidePrivilege::GET_DISCUSSION_THREAD_MESSAGES_OF_USER, "get_discussion_thread_messages_of_user" },
            { ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER, "get_subscribed_discussion_threads_of_user" },
            { ForumWidePrivilege::GET_ALL_DISCUSSION_CATEGORIES, "get_all_discussion_categories" },
            { ForumWidePrivilege::GET_DISCUSSION_CATEGORIES_FROM_ROOT, "get_discussion_categories_from_root" },
            { ForumWidePrivilege::GET_ALL_DISCUSSION_TAGS, "get_all_discussion_tags" },
            { ForumWidePrivilege::GET_ALL_DISCUSSION_THREADS, "get_all_discussion_threads" },
            { ForumWidePrivilege::GET_ALL_MESSAGE_COMMENTS, "get_all_message_comments" },
            { ForumWidePrivilege::GET_MESSAGE_COMMENTS_OF_USER, "get_message_comments_of_user" },
            { ForumWidePrivilege::ADD_DISCUSSION_CATEGORY, "add_discussion_category" },
            { ForumWidePrivilege::ADD_DISCUSSION_TAG, "add_discussion_tag" },
            { ForumWidePrivilege::ADD_DISCUSSION_THREAD, "add_discussion_thread" },
            { ForumWidePrivilege::CHANGE_OWN_USER_NAME, "change_own_user_name" },
            { ForumWidePrivilege::CHANGE_OWN_USER_INFO, "change_own_user_info" },
            { ForumWidePrivilege::CHANGE_ANY_USER_NAME, "change_any_user_name" },
            { ForumWidePrivilege::CHANGE_ANY_USER_INFO, "change_any_user_info" },
            { ForumWidePrivilege::DELETE_ANY_USER, "delete_any_user" },
            { ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, "adjust_forum_wide_privilege" }
        };

        enum class ForumWideDefaultPrivilegeDuration : EnumIntType
        {
            CHANGE_DISCUSSION_THREAD_NAME,
            DELETE_DISCUSSION_THREAD,

            COUNT
        };

        const StringView ForumWideDefaultPrivilegeDurationStrings[] =
        {
            "change_discussion_thread_name",
            "delete_discussion_thread"
        };

        typedef int16_t PrivilegeValueIntType;
        typedef boost::optional<PrivilegeValueIntType> PrivilegeValueType;
        typedef time_t PrivilegeDefaultDurationIntType;
        typedef boost::optional<PrivilegeDefaultDurationIntType> PrivilegeDefaultDurationType;

        static constexpr PrivilegeValueIntType MinPrivilegeValue = -32000;
        static constexpr PrivilegeValueIntType MaxPrivilegeValue = 32000;
        static constexpr PrivilegeDefaultDurationIntType UnlimitedDuration =
                std::numeric_limits<PrivilegeDefaultDurationIntType>::max();

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


        inline PrivilegeDefaultDurationType minimumPrivilegeDefaultDuration(PrivilegeDefaultDurationType first,
                                                                            PrivilegeDefaultDurationType second)
        {
            if ( ! first) return second;
            if ( ! second) return first;

            return std::min(*first, *second);
        }

        inline PrivilegeDefaultDurationType maximumPrivilegeDefaultDuration(PrivilegeDefaultDurationType first,
                                                                            PrivilegeDefaultDurationType second)
        {
            if ( ! first) return second;
            if ( ! second) return first;

            return std::max(*first, *second);
        }

        inline auto calculatePrivilegeExpires(PrivilegeDefaultDurationIntType start,
                                              PrivilegeDefaultDurationIntType duration)
        {
            static_assert(sizeof(PrivilegeDefaultDurationIntType) >= 8, "PrivilegeDefaultDurationIntType should be at least 64-bit wide");

            auto maxHalf = std::numeric_limits<PrivilegeDefaultDurationIntType>::max() / 2;
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
                    discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeValueType getDiscussionThreadMessagePrivilege(
                    DiscussionThreadMessagePrivilege privilege) const
            {
                if (privilege < DiscussionThreadMessagePrivilege::COUNT)
                {
                    return discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

        private:
            PrivilegeValueType discussionThreadMessagePrivileges_
                [static_cast<EnumIntType>(DiscussionThreadMessagePrivilege::COUNT)];
        };

        struct DiscussionThreadPrivilegeStore : public DiscussionThreadMessagePrivilegeStore
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

            void setDiscussionThreadMessageDefaultPrivilegeDuration(
                    DiscussionThreadMessageDefaultPrivilegeDuration privilege, PrivilegeDefaultDurationIntType value)
            {
                if (privilege < DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)
                {
                    discussionThreadMessageDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeDefaultDurationType getDiscussionThreadMessageDefaultPrivilegeDuration(
                    DiscussionThreadMessageDefaultPrivilegeDuration privilege) const
            {
                if (privilege < DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)
                {
                    return discussionThreadMessageDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

        private:
            PrivilegeValueType discussionThreadPrivileges_
                [static_cast<EnumIntType>(DiscussionThreadPrivilege::COUNT)];
            //default privilege durations for messages are stored on the parent thread
            //as the messages don't have settings the moment they are created
            PrivilegeDefaultDurationType discussionThreadMessageDefaultPrivilegeDuration_
                [static_cast<EnumIntType>(DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)];
        };

        struct DiscussionTagPrivilegeStore : public DiscussionThreadPrivilegeStore
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

        struct ForumWidePrivilegeStore : public DiscussionTagPrivilegeStore, public DiscussionCategoryPrivilegeStore
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

            void setForumWideDefaultPrivilegeDuration(ForumWideDefaultPrivilegeDuration privilege,
                                                      PrivilegeDefaultDurationIntType value)
            {
                if (privilege < ForumWideDefaultPrivilegeDuration::COUNT)
                {
                    forumWideDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeDefaultDurationType getForumWideDefaultPrivilegeDuration(
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
            PrivilegeDefaultDurationType forumWideDefaultPrivilegeDuration_
                [static_cast<EnumIntType>(ForumWideDefaultPrivilegeDuration::COUNT)];
        };
    }
}
