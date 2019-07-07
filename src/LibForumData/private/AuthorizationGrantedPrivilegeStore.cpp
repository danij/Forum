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

#include "AuthorizationGrantedPrivilegeStore.h"
#include "Configuration.h"
#include "EntityCollection.h"

#include <boost/range/iterator_range.hpp>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

GrantedPrivilegeStore::GrantedPrivilegeStore()
{
    defaultPrivilegeValueForLoggedInUser_ = Configuration::getGlobalConfig()->user.defaultPrivilegeValueForLoggedInUser;
    messageCountMultiplierPrivilegeBonus_ = Configuration::getGlobalConfig()->user.messageCountMultiplierPrivilegeBonus;
}

void GrantedPrivilegeStore::grantDiscussionThreadMessagePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                                  PrivilegeValueIntType value, Timestamp now,
                                                                  Timestamp expiresAt)
{
    if (0 == value)
    {
        const IdTuple toSearch{ userId, entityId };
        discussionThreadMessageSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionThreadMessageSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, now, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionThreadPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                           PrivilegeValueIntType value, Timestamp now,
                                                           Timestamp expiresAt)
{
    if (0 == value)
    {
        const IdTuple toSearch{ userId, entityId };
        discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionThreadSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, now, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionTagPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                        PrivilegeValueIntType value, Timestamp now,
                                                        Timestamp expiresAt)
{
    if (0 == value)
    {
        const IdTuple toSearch{ userId, entityId };
        discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionTagSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, now, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionCategoryPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                             PrivilegeValueIntType value, Timestamp now,
                                                             Timestamp expiresAt)
{
    if (0 == value)
    {
        const IdTuple toSearch{ userId, entityId };
        discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionCategorySpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, now, expiresAt));
}

void GrantedPrivilegeStore::grantForumWidePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                    PrivilegeValueIntType value, Timestamp now,
                                                    Timestamp expiresAt)
{
    if (0 == value)
    {
        const IdTuple toSearch{ userId, entityId };
        forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    forumWideSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, now, expiresAt));
}

static PrivilegeValueIntType getEffectivePrivilegeValue(const PrivilegeValueType positive, 
                                                        const PrivilegeValueType negative)
{
    return optionalOrZero(positive) + optionalOrZero(negative);
}

static PrivilegeValueType isAllowed(const PrivilegeValueType positive, 
                                    const PrivilegeValueType negative, const PrivilegeValueType required)
{
    const auto effectivePrivilegeValue = getEffectivePrivilegeValue(positive, negative);
    const auto requiredPrivilegeValue = optionalOrZero(required);

    return (effectivePrivilegeValue >= requiredPrivilegeValue) ? effectivePrivilegeValue : PrivilegeValueType{};
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionThreadMessage& message,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionThreadMessagePrivilege(user, message, now, positive, negative);

    DiscussionThreadConstPtr thread = message.parentThread();
    assert(thread);
    calculateDiscussionThreadMessagePrivilege(user, *thread, now, positive, negative);

    return ::isAllowed(positive, negative, message.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionThread& thread,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionThreadMessagePrivilege(user, thread, now, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionTag& tag,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionTagPrivilege(user, tag, now, positive, negative);
    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionThreadPrivilege(user, thread, now, positive, negative);

    for (auto tag : thread.tags())
    {
        assert(tag);
        calculateDiscussionTagPrivilege(user, *tag, now, positive, negative);
    }

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionTag& tag,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionTagPrivilege(user, tag, now, positive, negative);
    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionTagPrivilege(user, tag, now, positive, negative);

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionCategoryPrivilege(user, category, now, positive, negative);

    auto parent = category.parent();
    while (parent)
    {
        calculateDiscussionCategoryPrivilege(user, *parent, now, positive, negative);
        parent = parent->parent();
    }

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, category.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionTagPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionCategoryPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(UserConstPtr user, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    ForumWidePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateForumWidePrivilege(user, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getForumWidePrivilege(privilege));
}

void GrantedPrivilegeStore::computeDiscussionThreadMessageVisibilityAllowed(DiscussionThreadMessagePrivilegeCheck* items,
                                                                            size_t nrOfItems, Timestamp now) const
{
    if (nrOfItems < 1)
    {
        return;
    }

    struct PrivilegeInfo
    {
        DiscussionThreadMessagePrivilege privilege;
        PrivilegeValueType required{};
        bool* boolToUpdate{};

        PrivilegeInfo(DiscussionThreadMessagePrivilege value) : privilege(value) {}
    };

    PrivilegeInfo threadValues[] =
    {
        { DiscussionThreadMessagePrivilege::VIEW },
        { DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER },
        { DiscussionThreadMessagePrivilege::VIEW_VOTES },
        { DiscussionThreadMessagePrivilege::VIEW_IP_ADDRESS },
        { DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS },
    };

    auto& user = items[0].user;
    auto& firstMessage = *items[0].message;

    PrivilegeValueType threadLevelPositive{};
    PrivilegeValueType threadLevelNegative{};

    DiscussionThreadConstPtr thread = firstMessage.parentThread();
    assert(thread);

    //predetermine the privilege values granted and required at thread level as they are the same for all messages
    calculateDiscussionThreadMessagePrivilege(user, *thread, now, threadLevelPositive, threadLevelNegative);

    for (auto& info : threadValues)
    {
        info.required = thread->getDiscussionThreadMessagePrivilege(info.privilege);
    }

    for (size_t i = 0; i < nrOfItems; ++i)
    {
        auto& item = items[i];
        threadValues[0].boolToUpdate = &item.allowedToShowMessage;
        threadValues[1].boolToUpdate = &item.allowedToShowUser;
        threadValues[2].boolToUpdate = &item.allowedToShowVotes;
        threadValues[3].boolToUpdate = &item.allowedToShowIpAddress;
        threadValues[4].boolToUpdate = &item.allowedToViewComments;

        PrivilegeValueType messageLevelPositive, messageLevelNegative;
        calculateDiscussionThreadMessagePrivilege(item.user, *item.message, now,
                                                  messageLevelPositive, messageLevelNegative);
        const auto positive = maximumPrivilegeValue(messageLevelPositive, threadLevelPositive);
        const auto negative = minimumPrivilegeValue(messageLevelNegative, threadLevelNegative);

        for (auto& info : threadValues)
        {
            if ( ! item.message)
            {
                continue;
            }

            *(info.boolToUpdate) = static_cast<bool>(::isAllowed(positive, negative,
                    item.message->getDiscussionThreadMessagePrivilege(info.privilege, info.required)));
        }
    }
}

void GrantedPrivilegeStore::calculateDiscussionThreadMessagePrivilege(UserConstPtr user,
                                                                      const DiscussionThread& thread, Timestamp now,
                                                                      PrivilegeValueType& positiveValue,
                                                                      PrivilegeValueType& negativeValue) const
{
    calculateDiscussionThreadPrivilege(user, thread, now, positiveValue, negativeValue);
    for (auto tag : thread.tags())
    {
        assert(tag);
        calculateDiscussionTagPrivilege(user, *tag, now, positiveValue, negativeValue);
    }

    calculateForumWidePrivilege(user, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionThreadMessagePrivilege(UserConstPtr user,
                                                                      const DiscussionThreadMessage& message,
                                                                      Timestamp now,
                                                                      PrivilegeValueType& positiveValue,
                                                                      PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionThreadMessageSpecificPrivileges_, user, message.id(), now,
                       positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionThreadPrivilege(UserConstPtr user, const DiscussionThread& thread,
                                                               Timestamp now,
                                                               PrivilegeValueType& positiveValue,
                                                               PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionThreadSpecificPrivileges_, user, thread.id(), now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionTagPrivilege(UserConstPtr user, const DiscussionTag& tag, Timestamp now,
                                                            PrivilegeValueType& positiveValue,
                                                            PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionTagSpecificPrivileges_, user, tag.id(), now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionCategoryPrivilege(UserConstPtr user, const DiscussionCategory& category,
                                                                 Timestamp now,
                                                                 PrivilegeValueType& positiveValue,
                                                                 PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionCategorySpecificPrivileges_, user, category.id(), now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateForumWidePrivilege(UserConstPtr user, Timestamp now,
                                                        PrivilegeValueType& positiveValue,
                                                        PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(forumWideSpecificPrivileges_, user, {}, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculatePrivilege(const PrivilegeEntryCollection& collection, UserConstPtr user,
                                               IdTypeRef entityId, Timestamp now, PrivilegeValueType& positiveValue,
                                               PrivilegeValueType& negativeValue) const
{
    if (isAnonymousUser(user))
    {
        positiveValue = maximumPrivilegeValue(positiveValue, 0);
        return;
    }

    const auto defaultPositiveValue = static_cast<PrivilegeValueIntType>(
            defaultPrivilegeValueForLoggedInUser_ + messageCountMultiplierPrivilegeBonus_ * user->messageCount());

    positiveValue = maximumPrivilegeValue(positiveValue, defaultPositiveValue);
    
    auto range = collection.get<PrivilegeEntryCollectionByUserIdEntityId>().equal_range(IdTuple{ user->id(), entityId });
    for (auto& entry : boost::make_iterator_range(range))
    {
        const auto expiresAt = entry.expiresAt();
        if ((expiresAt > 0) && (expiresAt < now)) continue;

        const auto value = entry.privilegeValue();
        if (value > 0)
        {
            positiveValue = maximumPrivilegeValue(positiveValue, value);
        }
        else
        {
            negativeValue = minimumPrivilegeValue(negativeValue, value);
        }
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadMessagePrivileges(IdTypeRef id,
                                                                       EnumerationCallback&& callback) const
{
    auto range = discussionThreadMessageSpecificPrivileges_
            .get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadPrivileges(IdTypeRef id, EnumerationCallback&& callback) const
{
    auto range = discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionTagPrivileges(IdTypeRef id, EnumerationCallback&& callback) const

{
    auto range = discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionCategoryPrivileges(IdTypeRef id, EnumerationCallback&& callback) const

{
    auto range = discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateForumWidePrivileges(IdTypeRef /*id*/, EnumerationCallback&& callback) const
{
    auto range = forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(IdType{});

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadMessagePrivilegesAssignedToUser(IdTypeRef userId,
                                                                                     EnumerationCallback&& callback) const
{
    auto range = discussionThreadMessageSpecificPrivileges_
            .get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadPrivilegesAssignedToUser(IdTypeRef userId,
                                                                              EnumerationCallback&& callback) const
{
    auto range = discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionTagPrivilegesAssignedToUser(IdTypeRef userId,
                                                                           EnumerationCallback&& callback) const
{
    auto range = discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionCategoryPrivilegesAssignedToUser(IdTypeRef userId,
                                                                                EnumerationCallback&& callback) const
{
    auto range = discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateForumWidePrivilegesAssignedToUser(IdTypeRef userId,
                                                                       EnumerationCallback&& callback) const
{
    auto range = forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.grantedAt(), entry.expiresAt());
    }
}
