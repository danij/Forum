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
        struct DiscussionThreadMessagePrivilegeCheck
        {
            Entities::User& user;
            Entities::DiscussionThreadMessage& message;
            bool allowedToShowMessage;
            bool allowedToShowUser;
            bool allowedToShowVotes;
            bool allowedToShowIPAddress;
        };

        class GrantedPrivilegeStore final : private boost::noncopyable
        {
        public:
            //TODO remove expired privileges
            void grantDiscussionThreadMessagePrivilege(Entities::IdType userId, Entities::IdType entityId, 
                                                       DiscussionThreadMessagePrivilege privilege, 
                                                       PrivilegeValueIntType value, time_t expiresAt);
            
            void grantDiscussionThreadPrivilege(Entities::IdType userId, Entities::IdType entityId, 
                                                DiscussionThreadPrivilege privilege, 
                                                PrivilegeValueIntType value, time_t expiresAt);

            void grantDiscussionTagPrivilege(Entities::IdType userId, Entities::IdType entityId, 
                                             DiscussionTagPrivilege privilege, 
                                             PrivilegeValueIntType value, time_t expiresAt);

            void grantDiscussionCategoryPrivilege(Entities::IdType userId, Entities::IdType entityId, 
                                                  DiscussionCategoryPrivilege privilege, 
                                                  PrivilegeValueIntType value, time_t expiresAt);

            void grantForumWidePrivilege(Entities::IdType userId, Entities::IdType entityId, 
                                         ForumWidePrivilege privilege, 
                                         PrivilegeValueIntType value, time_t expiresAt);

            //isAllowed returns the privilege level with which access was granted or empty if not allowed

            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionThreadMessage& message, 
                                         DiscussionThreadMessagePrivilege privilege, time_t now) const;
            
            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionThread& thread,
                                         DiscussionThreadPrivilege privilege, time_t now) const;
            
            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionTag& tag,
                                         DiscussionTagPrivilege privilege, time_t now) const;
            
            PrivilegeValueType isAllowed(const Entities::User& user, const Entities::DiscussionCategory& category,
                                         DiscussionCategoryPrivilege privilege, time_t now) const;
            
            PrivilegeValueType isAllowed(Entities::IdType userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                         ForumWidePrivilege privilege, time_t now) const;

            void computeDiscussionThreadMessageVisibilityAllowed(DiscussionThreadMessagePrivilegeCheck* items,
                                                                 size_t nrOfItems, time_t now);
        private:

            void updateDiscussionThreadMessagePrivilege(const Entities::User& user, const Entities::DiscussionThread& thread, 
                                                        time_t now, DiscussionThreadMessagePrivilege privilege,
                                                        PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;
            
            void updateDiscussionThreadMessagePrivilege(Entities::IdType userId, Entities::IdType entityId, time_t now,
                                                        DiscussionThreadMessagePrivilege privilege,
                                                        PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;
            
            void updateDiscussionThreadPrivilege(Entities::IdType userId, Entities::IdType entityId, time_t now,
                                                 DiscussionThreadPrivilege privilege,
                                                 PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            void updateDiscussionTagPrivilege(Entities::IdType userId, Entities::IdType entityId, time_t now,
                                              DiscussionTagPrivilege privilege,
                                              PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const ;

            void updateDiscussionCategoryPrivilege(Entities::IdType userId, Entities::IdType entityId, time_t now,
                                                   DiscussionCategoryPrivilege privilege,
                                                   PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            void updateForumWidePrivilege(Entities::IdType userId, Entities::IdType entityId, time_t now,
                                          ForumWidePrivilege privilege,
                                          PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            typedef std::tuple<Entities::IdType, Entities::IdType, EnumIntType> IdPrivilegeTuple;

            struct PrivilegeEntry
            {
                PrivilegeEntry(Entities::IdType userId, Entities::IdType entityId,
                               EnumIntType privilege, PrivilegeValueIntType value, time_t expiresAt)
                    : userAndEntityAndPrivilege_({ userId, entityId, privilege }),
                      privilegeValue_(value), expiresAt_(expiresAt)
                {
                }

                const IdPrivilegeTuple& userEntityAndPrivilege() const { return userAndEntityAndPrivilege_; }

                Entities::IdType   userId() const { return std::get<0>(userAndEntityAndPrivilege_); }
                Entities::IdType entityId() const { return std::get<1>(userAndEntityAndPrivilege_); }
                auto            privilege() const { return std::get<2>(userAndEntityAndPrivilege_); }
                auto       privilegeValue() const { return privilegeValue_; }
                auto            expiresAt() const { return expiresAt_; }

            private:
                IdPrivilegeTuple userAndEntityAndPrivilege_;
                PrivilegeValueIntType privilegeValue_;
                time_t expiresAt_;
            };

            struct PrivilegeEntryCollectionByUserIdEntityIdPrivilege {};
            struct PrivilegeEntryCollectionByUserId {};
            struct PrivilegeEntryCollectionByEntityId {};

            struct PrivilegeEntryCollectionIndices : boost::multi_index::indexed_by<

                boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>,
                        const boost::multi_index::const_mem_fun<PrivilegeEntry, const IdPrivilegeTuple&, 
                                &PrivilegeEntry::userEntityAndPrivilege>>,

                boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByUserId>,
                        const boost::multi_index::const_mem_fun<PrivilegeEntry, Entities::IdType, &PrivilegeEntry::userId>>,

                boost::multi_index::hashed_non_unique<boost::multi_index::tag<PrivilegeEntryCollectionByEntityId>,
                        const boost::multi_index::const_mem_fun<PrivilegeEntry, Entities::IdType, &PrivilegeEntry::entityId>>
            > {};

            typedef boost::multi_index_container<PrivilegeEntry, PrivilegeEntryCollectionIndices>
                    PrivilegeEntryCollection;

            void updatePrivilege(const PrivilegeEntryCollection& collection, Entities::IdType userId, 
                                 Entities::IdType entityId, time_t now, EnumIntType privilege,
                                 PrivilegeValueType& positiveValue, PrivilegeValueType& negativeValue) const;

            PrivilegeEntryCollection discussionThreadMessageSpecificPrivileges_;
            PrivilegeEntryCollection discussionThreadSpecificPrivileges_;
            PrivilegeEntryCollection discussionTagSpecificPrivileges_;
            PrivilegeEntryCollection discussionCategorySpecificPrivileges_;
            PrivilegeEntryCollection forumWideSpecificPrivileges_;
        };

        struct SerializationRestriction final : private boost::noncopyable
        {
            SerializationRestriction(const GrantedPrivilegeStore& privilegeStore, const Entities::User& user, time_t now)
                : privilegeStore_(privilegeStore), user_(user), now_(now)
            {
            }

            bool isAllowed(const Entities::DiscussionThreadMessage& message) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, message, DiscussionThreadMessagePrivilege::VIEW, now_));
            }

            bool isAllowed(const Entities::DiscussionThread& thread) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, thread, DiscussionThreadPrivilege::VIEW, now_));
            }

            bool isAllowed(const Entities::DiscussionTag& tag) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, tag, DiscussionTagPrivilege::VIEW, now_));
            }

            bool isAllowed(const Entities::DiscussionCategory& category) const
            {
                return static_cast<bool>(privilegeStore_.isAllowed(user_, category, DiscussionCategoryPrivilege::VIEW, now_));
            }

        private:
            const GrantedPrivilegeStore& privilegeStore_;
            const Entities::User& user_;
            time_t now_;
        };
    }
}