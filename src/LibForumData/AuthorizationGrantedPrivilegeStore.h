#pragma once

#include "AuthorizationPrivileges.h"
#include "Entities.h"

#include <cstddef>
#include <ctime>
#include <tuple>

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum
{
    namespace Authorization
    {
        struct DiscussionThreadMessagePrivilegeCheck final
        {
            DiscussionThreadMessagePrivilegeCheck()
            {}

            DiscussionThreadMessagePrivilegeCheck(const Entities::User& user,
                                                  const Entities::DiscussionThreadMessage& message)
                : user(&user), message(&message)
            {
            }

            DiscussionThreadMessagePrivilegeCheck(const DiscussionThreadMessagePrivilegeCheck&) = default;
            DiscussionThreadMessagePrivilegeCheck(DiscussionThreadMessagePrivilegeCheck&&) = default;

            DiscussionThreadMessagePrivilegeCheck& operator=(const DiscussionThreadMessagePrivilegeCheck&) = default;
            DiscussionThreadMessagePrivilegeCheck& operator=(DiscussionThreadMessagePrivilegeCheck&&) = default;

            const Entities::User* user = nullptr;
            const Entities::DiscussionThreadMessage* message = nullptr;
            bool allowedToShowMessage = false;
            bool allowedToShowUser = false;
            bool allowedToShowVotes = false;
            bool allowedToShowIpAddress = false;
        };

        class GrantedPrivilegeStore final : private boost::noncopyable
        {
        public:
            //TODO remove expired privileges
            void grantDiscussionThreadMessagePrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                                       DiscussionThreadMessagePrivilege privilege,
                                                       PrivilegeValueIntType value, Entities::Timestamp expiresAt);

            void grantDiscussionThreadPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                                DiscussionThreadPrivilege privilege,
                                                PrivilegeValueIntType value, Entities::Timestamp expiresAt);

            void grantDiscussionTagPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                             DiscussionTagPrivilege privilege,
                                             PrivilegeValueIntType value, Entities::Timestamp expiresAt);

            void grantDiscussionCategoryPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                                  DiscussionCategoryPrivilege privilege,
                                                  PrivilegeValueIntType value, Entities::Timestamp expiresAt);

            void grantForumWidePrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                                         ForumWidePrivilege privilege,
                                         PrivilegeValueIntType value, Entities::Timestamp expiresAt);

            //isAllowed returns the privilege level with which access was granted or empty if not allowed

            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionThreadMessage& message,
                                         DiscussionThreadMessagePrivilege privilege, Entities::Timestamp now) const;

            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionThread& thread,
                                         DiscussionThreadPrivilege privilege, Entities::Timestamp now) const;

            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionTag& tag,
                                         DiscussionTagPrivilege privilege, Entities::Timestamp now) const;

            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionCategory& category,
                                         DiscussionCategoryPrivilege privilege, Entities::Timestamp now) const;

            PrivilegeValueType isAllowed(Entities::IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                         ForumWidePrivilege privilege, Entities::Timestamp now) const;

            void computeDiscussionThreadMessageVisibilityAllowed(DiscussionThreadMessagePrivilegeCheck* items,
                                                                 size_t nrOfItems, Entities::Timestamp now) const;
        private:

            void updateDiscussionThreadMessagePrivilege(const Entities::User& user, const Entities::DiscussionThread& thread,
                                                        Entities::Timestamp now, DiscussionThreadMessagePrivilege privilege,
                                                        PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            void updateDiscussionThreadMessagePrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId, Entities::Timestamp now,
                                                        DiscussionThreadMessagePrivilege privilege,
                                                        PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            void updateDiscussionThreadPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId, Entities::Timestamp now,
                                                 DiscussionThreadPrivilege privilege,
                                                 PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            void updateDiscussionTagPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId, Entities::Timestamp now,
                                              DiscussionTagPrivilege privilege,
                                              PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const ;

            void updateDiscussionCategoryPrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId, Entities::Timestamp now,
                                                   DiscussionCategoryPrivilege privilege,
                                                   PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            void updateForumWidePrivilege(Entities::IdTypeRef userId, Entities::IdTypeRef entityId, Entities::Timestamp now,
                                          ForumWidePrivilege privilege,
                                          PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            typedef std::tuple<Entities::IdType, Entities::IdType, EnumIntType> IdPrivilegeTuple;

            struct PrivilegeEntry
            {
                PrivilegeEntry(Entities::IdTypeRef userId, Entities::IdTypeRef entityId,
                               EnumIntType privilege, PrivilegeValueIntType value, Entities::Timestamp expiresAt)
                    : userAndEntityAndPrivilege_({ userId, entityId, privilege }),
                      privilegeValue_(value), expiresAt_(expiresAt)
                {
                }

                const IdPrivilegeTuple& userEntityAndPrivilege() const { return userAndEntityAndPrivilege_; }

                Entities::IdTypeRef   userId() const { return std::get<0>(userAndEntityAndPrivilege_); }
                Entities::IdTypeRef entityId() const { return std::get<1>(userAndEntityAndPrivilege_); }
                auto               privilege() const { return std::get<2>(userAndEntityAndPrivilege_); }
                auto          privilegeValue() const { return privilegeValue_; }
                auto               expiresAt() const { return expiresAt_; }

            private:
                IdPrivilegeTuple userAndEntityAndPrivilege_;
                PrivilegeValueIntType privilegeValue_;
                Entities::Timestamp expiresAt_;
            };

            struct PrivilegeEntryCollectionByUserIdEntityIdPrivilege {};
            struct PrivilegeEntryCollectionByUserId {};
            struct PrivilegeEntryCollectionByEntityId {};

            struct PrivilegeEntryCollectionIndices : boost::multi_index::indexed_by<

                boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>,
                        const boost::multi_index::const_mem_fun<PrivilegeEntry, const IdPrivilegeTuple&,
                                &PrivilegeEntry::userEntityAndPrivilege>>,

                boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByUserId>,
                        const boost::multi_index::const_mem_fun<PrivilegeEntry, Entities::IdTypeRef, &PrivilegeEntry::userId>>,

                boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByEntityId>,
                        const boost::multi_index::const_mem_fun<PrivilegeEntry, Entities::IdTypeRef, &PrivilegeEntry::entityId>>
            > {};

            typedef boost::multi_index_container<PrivilegeEntry, PrivilegeEntryCollectionIndices>
                    PrivilegeEntryCollection;

            void updatePrivilege(const PrivilegeEntryCollection& collection, Entities::IdTypeRef userId,
                                 Entities::IdTypeRef entityId, Entities::Timestamp now, EnumIntType privilege,
                                 PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            PrivilegeEntryCollection discussionThreadMessageSpecificPrivileges_;
            PrivilegeEntryCollection discussionThreadSpecificPrivileges_;
            PrivilegeEntryCollection discussionTagSpecificPrivileges_;
            PrivilegeEntryCollection discussionCategorySpecificPrivileges_;
            PrivilegeEntryCollection forumWideSpecificPrivileges_;
        };

        struct SerializationRestriction final : private boost::noncopyable
        {
            SerializationRestriction(const GrantedPrivilegeStore& privilegeStore, const Entities::User& user, Entities::Timestamp now)
                : privilegeStore_(privilegeStore), user_(user), now_(now)
            {
            }

            const GrantedPrivilegeStore& privilegeStore() const { return privilegeStore_; }
            const        Entities::User&           user() const { return user_; }
                                 Entities::Timestamp             now() const { return now_; }

            bool isAllowed(const Entities::DiscussionThreadMessage& message,
                           DiscussionThreadMessagePrivilege privilege = DiscussionThreadMessagePrivilege::VIEW) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, message, privilege, now_));
            }

            bool isAllowed(const Entities::DiscussionThread& thread,
                           DiscussionThreadPrivilege privilege = DiscussionThreadPrivilege::VIEW) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, thread, privilege, now_));
            }

            bool isAllowed(const Entities::DiscussionTag& tag,
                           DiscussionTagPrivilege privilege = DiscussionTagPrivilege::VIEW) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, tag, privilege, now_));
            }

            bool isAllowed(const Entities::DiscussionCategory& category,
                           DiscussionCategoryPrivilege privilege = DiscussionCategoryPrivilege::VIEW) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, category, privilege, now_));
            }

        private:
            const GrantedPrivilegeStore& privilegeStore_;
            const Entities::User& user_;
            Entities::Timestamp now_;
        };
    }
}
