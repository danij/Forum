#pragma once

#include "TypeHelpers.h"

#include <algorithm>
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
            VIEW_DISCUSSION_THREAD_MESSAGE,
            UP_VOTE_DISCUSSION_THREAD_MESSAGE,
            DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
            RESET_VOTE_DISCUSSION_THREAD_MESSAGE,
            ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE,
            SET_COMMENT_TO_SOLVED_IN_DISCUSSION_THREAD_MESSAGE,
            GET_MESSAGE_COMMENTS,
            CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
            DELETE_DISCUSSION_THREAD_MESSAGE,
            MOVE_DISCUSSION_MESSAGE,

            COUNT
        };

        enum class DiscussionThreadMessageDefaultPrivilegeDuration : EnumIntType
        {
            RESET_VOTE_DISCUSSION_THREAD_MESSAGE,
            CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
            DELETE_DISCUSSION_THREAD_MESSAGE,

            COUNT
        };

        enum class DiscussionThreadPrivilege : EnumIntType
        {
            VIEW_DISCUSSION_THREAD,
            SUBSCRIBE_TO_THREAD,
            UNSUBSCRIBE_FROM_THREAD,
            ADD_MESSAGE_TO_DISCUSSION_THREAD,
            CHANGE_DISCUSSION_THREAD_NAME,
            ADD_TAG_TO_DISCUSSION_THREAD,
            REMOVE_TAG_TO_DISCUSSION_THREAD,
            DELETE_DISCUSSION_THREAD,
            MERGE_DISCUSSION_THREADS,
            
            COUNT
        };

        enum class DiscussionThreadDefaultPrivilegeDuration : EnumIntType
        {
            CHANGE_DISCUSSION_THREAD_NAME,
            DELETE_DISCUSSION_THREAD,

            COUNT
        };

        enum class DiscussionTagPrivilege : EnumIntType
        {
            VIEW_DISCUSSION_TAG,
            GET_DISCUSSION_THREADS_OF_TAG,
            GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER,
            CHANGE_DISCUSSION_TAG_NAME,
            CHANGE_DISCUSSION_TAG_UIBLOB,
            DELETE_DISCUSSION_TAG,
            MERGE_DISCUSSION_TAGS,
            
            COUNT
        };
        
        enum class DiscussionCategoryPrivilege : EnumIntType
        {
            VIEW_DISCUSSION_CATEGORY,
            CHANGE_DISCUSSION_CATEGORY_NAME,
            CHANGE_DISCUSSION_CATEGORY_DESCRIPTION,
            CHANGE_DISCUSSION_CATEGORY_PARENT,
            CHANGE_DISCUSSION_CATEGORY_DISPLAYORDER,
            ADD_TAG_TO_DISCUSSION_CATEGORY,
            REMOVE_TAG_FROM_DISCUSSION_CATEGORY,
            DELETE_DISCUSSION_CATEGORY,

            COUNT
        };

        enum class ForumWidePrivilege : EnumIntType
        {
            LOGIN,
            GET_ENTITIES_COUNT,
            LIST_USERS,
            GET_USER_INFO,
            GET_DISCUSSION_THREADS_OF_USER,
            GET_DISCUSSION_THREAD_MESSAGES_OF_USER,
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

            COUNT
        };

        typedef int16_t PrivilegeValueIntType;
        typedef boost::optional<PrivilegeValueIntType> PrivilegeValueType;
        typedef time_t PrivilegeDefaultDurationIntType;
        typedef boost::optional<PrivilegeDefaultDurationIntType> PrivilegeDefaultDurationType;

        inline PrivilegeValueType MinimumPrivilegeValue(PrivilegeValueType first, PrivilegeValueType second)
        {
            if ( ! first) return second;
            if ( ! second) return first;

            return std::min(*first, *second);
        }

        inline PrivilegeValueType MaximumPrivilegeValue(PrivilegeValueType first, PrivilegeValueType second)
        {
            if ( ! first) return second;
            if ( ! second) return first;

            return std::max(*first, *second);
        }

        inline PrivilegeDefaultDurationType MinimumPrivilegeDefaultDuration(PrivilegeDefaultDurationType first, 
                                                                            PrivilegeDefaultDurationType second)
        {
            if ( ! first) return second;
            if ( ! second) return first;

            return std::min(*first, *second);
        }

        inline PrivilegeDefaultDurationType MaximumPrivilegeDefaultDuration(PrivilegeDefaultDurationType first, 
                                                                            PrivilegeDefaultDurationType second)
        {
            if ( ! first) return second;
            if ( ! second) return first;

            return std::max(*first, *second);
        }

        class DiscussionThreadMessagePrivilegeStore
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(DiscussionThreadMessagePrivilegeStore)

            void setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege, 
                                                     PrivilegeValueIntType value)
            {
                if (privilege < DiscussionThreadMessagePrivilege::COUNT)
                {
                    discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeValueType getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege)
            {
                if (privilege < DiscussionThreadMessagePrivilege::COUNT)
                {
                    return discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

            void setDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                                                                    PrivilegeDefaultDurationIntType value)
            {
                if (privilege < DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)
                {
                    discussionThreadMessagePrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeDefaultDurationType getDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege)
            {
                if (privilege < DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)
                {
                    return discussionThreadMessageDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

        private:
            PrivilegeValueType discussionThreadMessagePrivileges_
                [static_cast<EnumIntType>(DiscussionThreadMessagePrivilege::COUNT)];
            PrivilegeDefaultDurationType discussionThreadMessageDefaultPrivilegeDuration_
                [static_cast<EnumIntType>(DiscussionThreadMessageDefaultPrivilegeDuration::COUNT)];
        };

       class DiscussionThreadPrivilegeStore : public DiscussionThreadMessagePrivilegeStore
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(DiscussionThreadPrivilegeStore)

            void setDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege, 
                                              PrivilegeValueIntType value)
            {
                if (privilege < DiscussionThreadPrivilege::COUNT)
                {
                    discussionThreadPrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeValueType getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege)
            {
                if (privilege < DiscussionThreadPrivilege::COUNT)
                {
                    return discussionThreadPrivileges_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

            void setDiscussionThreadDefaultPrivilegeDuration(DiscussionThreadDefaultPrivilegeDuration privilege,
                                                             PrivilegeDefaultDurationIntType value)
            {
                if (privilege < DiscussionThreadDefaultPrivilegeDuration::COUNT)
                {
                    discussionThreadPrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeDefaultDurationType getDiscussionThreadDefaultPrivilegeDuration(DiscussionThreadDefaultPrivilegeDuration privilege)
            {
                if (privilege < DiscussionThreadDefaultPrivilegeDuration::COUNT)
                {
                    return discussionThreadDefaultPrivilegeDuration_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

        private:
            PrivilegeValueType discussionThreadPrivileges_
                [static_cast<EnumIntType>(DiscussionThreadPrivilege::COUNT)];
            PrivilegeDefaultDurationType discussionThreadDefaultPrivilegeDuration_
                [static_cast<EnumIntType>(DiscussionThreadDefaultPrivilegeDuration::COUNT)];
        };

        class DiscussionTagPrivilegeStore : public DiscussionThreadPrivilegeStore
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(DiscussionTagPrivilegeStore)

            void setDiscussionTagPrivilege(DiscussionTagPrivilege privilege, PrivilegeValueIntType value)
            {
                if (privilege < DiscussionTagPrivilege::COUNT)
                {
                    discussionTagPrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeValueType getDiscussionTagPrivilege(DiscussionTagPrivilege privilege)
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

        class DiscussionCategoryPrivilegeStore
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(DiscussionCategoryPrivilegeStore)

            void setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value)
            {
                if (privilege < DiscussionCategoryPrivilege::COUNT)
                {
                    discussionCategoryPrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeValueType getDiscussionCategoryPrivilege(DiscussionCategoryPrivilege privilege)
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

        class ForumWidePrivilegeStore : public DiscussionTagPrivilegeStore, public DiscussionCategoryPrivilegeStore
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(ForumWidePrivilegeStore)

            void setForumWidePrivilege(ForumWidePrivilege privilege, PrivilegeValueIntType value)
            {
                if (privilege < ForumWidePrivilege::COUNT)
                {
                    forumWidePrivileges_[static_cast<EnumIntType>(privilege)] = value;
                }
            }

            virtual PrivilegeValueType getDForumWidePrivilege(ForumWidePrivilege privilege)
            {
                if (privilege < ForumWidePrivilege::COUNT)
                {
                    return forumWidePrivileges_[static_cast<EnumIntType>(privilege)];
                }
                return{};
            }

        private:
            PrivilegeValueType forumWidePrivileges_ [static_cast<EnumIntType>(ForumWidePrivilege::COUNT)];
        };
    }
}
