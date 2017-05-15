#pragma once

#include "TypeHelpers.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ctime>
#include <tuple>

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

        const std::array<std::tuple<DiscussionThreadMessagePrivilege, StringView>, 4> DiscussionThreadMessagePrivilegesToSerialize =
        {
            std::tuple<DiscussionThreadMessagePrivilege, StringView>{ DiscussionThreadMessagePrivilege::UP_VOTE, "up_vote" },
            std::tuple<DiscussionThreadMessagePrivilege, StringView>{ DiscussionThreadMessagePrivilege::DOWN_VOTE, "down_vote" },
            std::tuple<DiscussionThreadMessagePrivilege, StringView>{ DiscussionThreadMessagePrivilege::CHANGE_CONTENT, "change_content" },
            std::tuple<DiscussionThreadMessagePrivilege, StringView>{ DiscussionThreadMessagePrivilege::DELETE, "delete" }
        };

        enum class DiscussionThreadMessageDefaultPrivilegeDuration : EnumIntType
        {
            RESET_VOTE,
            CHANGE_CONTENT,
            DELETE,

            COUNT
        };

        enum class DiscussionThreadPrivilege : EnumIntType
        {
            VIEW,
            SUBSCRIBE,
            UNSUBSCRIBE,
            ADD_MESSAGE,
            CHANGE_NAME,
            ADD_TAG,
            REMOVE_TAG,
            DELETE,
            MERGE,

            ADJUST_PRIVILEGE,

            COUNT
        };

        const std::array<std::tuple<DiscussionThreadPrivilege, StringView>, 5> DiscussionThreadPrivilegesToSerialize =
        {
            std::tuple<DiscussionThreadPrivilege, StringView>{ DiscussionThreadPrivilege::ADD_MESSAGE, "add_message" },
            std::tuple<DiscussionThreadPrivilege, StringView>{ DiscussionThreadPrivilege::CHANGE_NAME, "change_name" },
            std::tuple<DiscussionThreadPrivilege, StringView>{ DiscussionThreadPrivilege::ADD_TAG, "add_tag" },
            std::tuple<DiscussionThreadPrivilege, StringView>{ DiscussionThreadPrivilege::REMOVE_TAG, "remove_tag" },
            std::tuple<DiscussionThreadPrivilege, StringView>{ DiscussionThreadPrivilege::DELETE, "delete" },
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

        const std::array<std::tuple<DiscussionTagPrivilege, StringView>, 3> DiscussionTagPrivilegesToSerialize =
        {
            std::tuple<DiscussionTagPrivilege, StringView>{ DiscussionTagPrivilege::CHANGE_NAME, "change_name" },
            std::tuple<DiscussionTagPrivilege, StringView>{ DiscussionTagPrivilege::CHANGE_UIBLOB, "change_uiblob" },
            std::tuple<DiscussionTagPrivilege, StringView>{ DiscussionTagPrivilege::DELETE, "delete" }
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

        const std::array<std::tuple<DiscussionCategoryPrivilege, StringView>, 7> DiscussionCategoryPrivilegesToSerialize = 
        {
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::CHANGE_NAME, "change_name" },
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::CHANGE_DESCRIPTION, "change_description" },
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::CHANGE_PARENT, "change_parent" },
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER, "change_displayorder" },
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::ADD_TAG, "add_tag" },
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::REMOVE_TAG, "remove_tag" },
            std::tuple<DiscussionCategoryPrivilege, StringView>{ DiscussionCategoryPrivilege::DELETE, "delete" }
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
            CHANGE_ANY_USER_NAME,
            CHANGE_ANY_USER_INFO,
            DELETE_ANY_USER,

            ADJUST_FORUM_WIDE_PRIVILEGE,

            COUNT
        };

        enum class ForumWideDefaultPrivilegeDuration : EnumIntType
        {
            CHANGE_DISCUSSION_THREAD_NAME,
            DELETE_DISCUSSION_THREAD,

            COUNT
        };

        typedef int16_t PrivilegeValueIntType;
        typedef boost::optional<PrivilegeValueIntType> PrivilegeValueType;
        typedef time_t PrivilegeDefaultDurationIntType;
        typedef boost::optional<PrivilegeDefaultDurationIntType> PrivilegeDefaultDurationType;

        static constexpr PrivilegeValueIntType MinPrivilegeValue = -32000;
        static constexpr PrivilegeValueIntType MaxPrivilegeValue = 32000;

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

            virtual PrivilegeValueType getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
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

            void setDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                PrivilegeDefaultDurationIntType value)
            {
                if (privilege < DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)
                {
                    discussionThreadMessageDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeDefaultDurationType getDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege) const
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

            virtual PrivilegeDefaultDurationType getForumWideDefaultPrivilegeDuration(ForumWideDefaultPrivilegeDuration privilege) const
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