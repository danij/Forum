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
}

void GrantedPrivilegeStore::grantDiscussionThreadMessagePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                                  PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdTuple toSearch{ userId, entityId };
        discussionThreadMessageSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionThreadMessageSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionThreadPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                           PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdTuple toSearch{ userId, entityId };
        discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionThreadSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionTagPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                        PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdTuple toSearch{ userId, entityId };
        discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionTagSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionCategoryPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                             PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdTuple toSearch{ userId, entityId };
        discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    discussionCategorySpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, expiresAt));
}

void GrantedPrivilegeStore::grantForumWidePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                    PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdTuple toSearch{ userId, entityId };
        forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityId>().erase(toSearch);
        return;
    }
    forumWideSpecificPrivileges_.insert(PrivilegeEntry(userId, entityId, value, expiresAt));
}

static PrivilegeValueIntType getEffectivePrivilegeValue(PrivilegeValueType positive, PrivilegeValueType negative)
{
    return optionalOrZero(positive) - optionalOrZero(negative);
}

static PrivilegeValueType isAllowed(PrivilegeValueType positive, PrivilegeValueType negative, PrivilegeValueType required)
{
    auto effectivePrivilegeValue = getEffectivePrivilegeValue(positive, negative);
    auto requiredPrivilegeValue = optionalOrZero(required);

    return (effectivePrivilegeValue >= requiredPrivilegeValue) ? effectivePrivilegeValue : PrivilegeValueType{};
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionThreadMessage& message,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionThreadMessagePrivilege(userId, message.id(), now, positive, negative);

    DiscussionThreadConstPtr thread = message.parentThread();
    assert(thread);
    calculateDiscussionThreadMessagePrivilege(userId, *thread, now, positive, negative);

    return ::isAllowed(positive, negative, message.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionThread& thread,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionThreadMessagePrivilege(userId, thread, now, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionThreadMessagePrivilege(userId, tag.id(), now, positive, negative);
    calculateDiscussionThreadMessagePrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionThreadPrivilege(userId, thread.id(), now, positive, negative);

    for (auto tag : thread.tags())
    {
        assert(tag);
        calculateDiscussionThreadPrivilege(userId, tag->id(), now, positive, negative);
    }

    calculateDiscussionThreadPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionThreadPrivilege(userId, tag.id(), now, positive, negative);
    calculateDiscussionThreadPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionTagPrivilege(userId, tag.id(), now, positive, negative);

    calculateDiscussionTagPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    calculateDiscussionCategoryPrivilege(userId, category.id(), now, positive, negative);

    auto parent = category.parent();
    while (parent)
    {
        calculateDiscussionCategoryPrivilege(userId, parent->id(), now, positive, negative);
        parent = parent->parent();
    }

    calculateDiscussionCategoryPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, category.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionThreadMessagePrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionThreadPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionTagPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionTagPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionCategoryPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateDiscussionCategoryPrivilege(userId, {}, now, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    ForumWidePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    calculateForumWidePrivilege(userId, now, positive, negative);

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
    };

    auto& userId = items[0].userId;
    auto& firstMessage = *items[0].message;

    PrivilegeValueType threadLevelPositive{};
    PrivilegeValueType threadLevelNegative{};

    DiscussionThreadConstPtr thread = firstMessage.parentThread();
    assert(thread);

    //predetermine the privilege values granted and required at thread level as they are the same for all messages
    calculateDiscussionThreadMessagePrivilege(userId, *thread, now, threadLevelPositive, threadLevelNegative);

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

        PrivilegeValueType messageLevelPositive, messageLevelNegative;
        calculateDiscussionThreadMessagePrivilege(item.userId, item.message->id(), now,
                                                  messageLevelPositive, messageLevelNegative);
        auto positive = maximumPrivilegeValue(messageLevelPositive, threadLevelPositive);
        auto negative = minimumPrivilegeValue(messageLevelNegative, threadLevelNegative);

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

void GrantedPrivilegeStore::calculateDiscussionThreadMessagePrivilege(IdTypeRef userId,
                                                                      const DiscussionThread& thread, Timestamp now,
                                                                      PrivilegeValueType& positiveValue,
                                                                      PrivilegeValueType& negativeValue) const
{
    calculateDiscussionThreadMessagePrivilege(userId, thread.id(), now, positiveValue, negativeValue);
    for (auto tag : thread.tags())
    {
        assert(tag);
        calculateDiscussionThreadMessagePrivilege(userId, tag->id(), now, positiveValue, negativeValue);
    }

    calculateDiscussionThreadMessagePrivilege(userId, {}, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionThreadMessagePrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                                      PrivilegeValueType& positiveValue,
                                                                      PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionThreadMessageSpecificPrivileges_, userId, entityId, now,
                       positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionThreadPrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                               PrivilegeValueType& positiveValue,
                                                               PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionThreadSpecificPrivileges_, userId, entityId, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionTagPrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                            PrivilegeValueType& positiveValue,
                                                            PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionTagSpecificPrivileges_, userId, entityId, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateDiscussionCategoryPrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                                 PrivilegeValueType& positiveValue,
                                                                 PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(discussionCategorySpecificPrivileges_, userId, entityId, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculateForumWidePrivilege(IdTypeRef userId, Timestamp now,
                                                        PrivilegeValueType& positiveValue,
                                                        PrivilegeValueType& negativeValue) const
{
    calculatePrivilege(forumWideSpecificPrivileges_, userId, {}, now, positiveValue, negativeValue);
}

void GrantedPrivilegeStore::calculatePrivilege(const PrivilegeEntryCollection& collection, IdTypeRef userId,
                                               IdTypeRef entityId, Timestamp now, PrivilegeValueType& positiveValue,
                                               PrivilegeValueType& negativeValue) const
{
    IdTuple toSearch{ userId, entityId };
    auto range = collection.get<PrivilegeEntryCollectionByUserIdEntityId>().equal_range(toSearch);

    positiveValue = (userId == anonymousUserId())
                    ? static_cast<PrivilegeValueIntType>(0)
                    : defaultPrivilegeValueForLoggedInUser_;

    for (auto& entry : boost::make_iterator_range(range))
    {
        auto expiresAt = entry.expiresAt();
        if ((expiresAt > 0) && (expiresAt < now)) continue;

        auto value = entry.privilegeValue();

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
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadMessageSpecificPrivileges_
            .get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadPrivileges(IdTypeRef id,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionTagPrivileges(IdTypeRef id,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionCategoryPrivileges(IdTypeRef id,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateForumWidePrivileges(IdTypeRef _,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(IdType{});

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadMessagePrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadMessageSpecificPrivileges_
            .get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadPrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionTagPrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionCategoryPrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateForumWidePrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilegeValue(), entry.expiresAt());
    }
}
