#include "AuthorizationGrantedPrivilegeStore.h"

#include <boost/range/iterator_range.hpp>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

void GrantedPrivilegeStore::grantDiscussionThreadMessagePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                                  DiscussionThreadMessagePrivilege privilege,
                                                                  PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdPrivilegeTuple toSearch{ userId, entityId, static_cast<EnumIntType>(privilege) };
        discussionThreadMessageSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>().erase(toSearch);
        return;
    }
    discussionThreadMessageSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionThreadPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                           DiscussionThreadPrivilege privilege,
                                                           PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdPrivilegeTuple toSearch{ userId, entityId, static_cast<EnumIntType>(privilege) };
        discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>().erase(toSearch);
        return;
    }
    discussionThreadSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionTagPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                        DiscussionTagPrivilege privilege,
                                                        PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdPrivilegeTuple toSearch{ userId, entityId, static_cast<EnumIntType>(privilege) };
        discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>().erase(toSearch);
        return;
    }
    discussionTagSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionCategoryPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                             DiscussionCategoryPrivilege privilege,
                                                             PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdPrivilegeTuple toSearch{ userId, entityId, static_cast<EnumIntType>(privilege) };
        discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>().erase(toSearch);
        return;
    }
    discussionCategorySpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantForumWidePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                    ForumWidePrivilege privilege,
                                                    PrivilegeValueIntType value, Timestamp expiresAt)
{
    if (0 == value)
    {
        IdPrivilegeTuple toSearch{ userId, entityId, static_cast<EnumIntType>(privilege) };
        forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>().erase(toSearch);
        return;
    }
    forumWideSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

static PrivilegeValueIntType getEffectivePrivilegeValue(PrivilegeValueType positive, PrivilegeValueType negative)
{
    return optionalOrZero(positive) - optionalOrZero(negative);
}

void GrantedPrivilegeStore::updateDiscussionThreadMessagePrivilege(IdTypeRef userId,
                                                                   const DiscussionThread& thread, Timestamp now,
                                                                   DiscussionThreadMessagePrivilege privilege,
                                                                   PrivilegeValueType& positiveValue,
                                                                   PrivilegeValueType& negativeValue) const
{
    updateDiscussionThreadMessagePrivilege(userId, thread.id(), now, privilege, positiveValue, negativeValue);
    for (auto tag : thread.tags())
    {
        assert(tag);
        updateDiscussionThreadMessagePrivilege(userId, tag->id(), now, privilege, positiveValue, negativeValue);
    }

    updateDiscussionThreadMessagePrivilege(userId, {}, now, privilege, positiveValue, negativeValue);
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
    updateDiscussionThreadMessagePrivilege(userId, message.id(), now, privilege, positive, negative);

    DiscussionThreadConstPtr thread = message.parentThread();
    assert(thread);
    updateDiscussionThreadMessagePrivilege(userId, *thread, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, message.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionThread& thread,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionThreadMessagePrivilege(userId, thread, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    
    updateDiscussionThreadMessagePrivilege(userId, tag.id(), now, privilege, positive, negative);
    updateDiscussionThreadMessagePrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionThreadPrivilege(userId, thread.id(), now, privilege, positive, negative);

    for (auto tag : thread.tags())
    {
        assert(tag);
        updateDiscussionThreadPrivilege(userId, tag->id(), now, privilege, positive, negative);
    }

    updateDiscussionThreadPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    updateDiscussionThreadPrivilege(userId, tag.id(), now, privilege, positive, negative);
    updateDiscussionThreadPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionTagPrivilege(userId, tag.id(), now, privilege, positive, negative);

    updateDiscussionTagPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionCategoryPrivilege(userId, category.id(), now, privilege, positive, negative);

    updateDiscussionCategoryPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, category.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionThreadMessagePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    updateDiscussionThreadMessagePrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    updateDiscussionThreadPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionTagPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    updateDiscussionTagPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    DiscussionCategoryPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    updateDiscussionCategoryPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdTypeRef userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    ForumWidePrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;

    updateForumWidePrivilege(userId, {}, now, privilege, positive, negative);

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
        PrivilegeValueType positive{};
        PrivilegeValueType negative{};
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

    DiscussionThreadConstPtr thread = firstMessage.parentThread();
    assert(thread);
    //predetermine the privilege values granted and required at thread level as they are the same for all messages
    for (auto& info : threadValues)
    {
        updateDiscussionThreadMessagePrivilege(userId, *thread, now, info.privilege, info.positive, info.negative);

        info.required = thread->getDiscussionThreadMessagePrivilege(info.privilege);
    }

    for (size_t i = 0; i < nrOfItems; ++i)
    {
        auto& item = items[i];
        threadValues[0].boolToUpdate = &item.allowedToShowMessage;
        threadValues[1].boolToUpdate = &item.allowedToShowUser;
        threadValues[2].boolToUpdate = &item.allowedToShowVotes;
        threadValues[3].boolToUpdate = &item.allowedToShowIpAddress;

        for (auto& info : threadValues)
        {
            PrivilegeValueType messageLevelPositive, messageLevelNegative;

            if ( ! item.message)
            {
                continue;
            }

            updateDiscussionThreadMessagePrivilege(item.userId, item.message->id(), now, info.privilege,
                                                   messageLevelPositive, messageLevelNegative);

            auto positive = maximumPrivilegeValue(messageLevelPositive, info.positive);
            auto negative = minimumPrivilegeValue(messageLevelNegative, info.negative);

            *(info.boolToUpdate) = static_cast<bool>(::isAllowed(positive, negative, 
                    item.message->getDiscussionThreadMessagePrivilege(info.privilege, info.required)));
        }
    }
}

void GrantedPrivilegeStore::updateDiscussionThreadMessagePrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                                   DiscussionThreadMessagePrivilege privilege,
                                                                   PrivilegeValueType& positiveValue,
                                                                   PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionThreadMessageSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege),
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateDiscussionThreadPrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                            DiscussionThreadPrivilege privilege,
                                                            PrivilegeValueType& positiveValue,
                                                            PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionThreadSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege),
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateDiscussionTagPrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                         DiscussionTagPrivilege privilege,
                                                         PrivilegeValueType& positiveValue,
                                                         PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionTagSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege),
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateDiscussionCategoryPrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                              DiscussionCategoryPrivilege privilege,
                                                              PrivilegeValueType& positiveValue,
                                                              PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionCategorySpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege),
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateForumWidePrivilege(IdTypeRef userId, IdTypeRef entityId, Timestamp now,
                                                     ForumWidePrivilege privilege,
                                                     PrivilegeValueType& positiveValue,
                                                     PrivilegeValueType& negativeValue) const
{
    updatePrivilege(forumWideSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege),
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updatePrivilege(const PrivilegeEntryCollection& collection, IdTypeRef userId, IdTypeRef entityId,
                                            Timestamp now, EnumIntType privilege, PrivilegeValueType& positiveValue,
                                            PrivilegeValueType& negativeValue) const
{
    IdPrivilegeTuple toSearch{ userId, entityId, privilege };
    auto range = collection.get<PrivilegeEntryCollectionByUserIdEntityIdPrivilege>().equal_range(toSearch);

    for (auto& entry : boost::make_iterator_range(range))
    {
        auto expiresAt = entry.expiresAt();
        if ((expiresAt > 0) && (expiresAt < now)) continue;

        auto value = entry.privilegeValue();

        if (value >= 0)
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
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadMessageSpecificPrivileges_
            .get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadPrivileges(IdTypeRef id,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionTagPrivileges(IdTypeRef id,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionCategoryPrivileges(IdTypeRef id,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(id);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateForumWidePrivileges(IdTypeRef _,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByEntityId>().equal_range(IdType{});

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.userId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadMessagePrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadMessageSpecificPrivileges_
            .get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionThreadPrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionThreadSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionTagPrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionTagSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateDiscussionCategoryPrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = discussionCategorySpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}

void GrantedPrivilegeStore::enumerateForumWidePrivilegesAssignedToUser(IdTypeRef userId,
        std::function<void(IdTypeRef, EnumIntType, PrivilegeValueIntType, Timestamp)>&& callback) const
{
    auto range = forumWideSpecificPrivileges_.get<PrivilegeEntryCollectionByUserId>().equal_range(userId);

    for (const PrivilegeEntry& entry : boost::make_iterator_range(range))
    {
        callback(entry.entityId(), entry.privilege(), entry.privilegeValue(), entry.expiresAt());
    }
}
