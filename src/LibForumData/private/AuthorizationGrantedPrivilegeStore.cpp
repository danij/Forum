#include "AuthorizationGrantedPrivilegeStore.h"

#include <boost/range/iterator_range.hpp>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

void GrantedPrivilegeStore::grantDiscussionThreadMessagePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                                  DiscussionThreadMessagePrivilege privilege,
                                                                  PrivilegeValueIntType value, Timestamp expiresAt)
{
    discussionThreadMessageSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionThreadPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                           DiscussionThreadPrivilege privilege,
                                                           PrivilegeValueIntType value, Timestamp expiresAt)
{
    discussionThreadSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionTagPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                        DiscussionTagPrivilege privilege,
                                                        PrivilegeValueIntType value, Timestamp expiresAt)
{
    discussionTagSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionCategoryPrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                             DiscussionCategoryPrivilege privilege,
                                                             PrivilegeValueIntType value, Timestamp expiresAt)
{
    discussionCategorySpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantForumWidePrivilege(IdTypeRef userId, IdTypeRef entityId,
                                                    ForumWidePrivilege privilege,
                                                    PrivilegeValueIntType value, Timestamp expiresAt)
{
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
        if (tag)
        {
            updateDiscussionThreadMessagePrivilege(userId, tag->id(), now, privilege, positiveValue, negativeValue);
        }
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
                                                    DiscussionThreadPrivilege privilege, Timestamp now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionThreadPrivilege(userId, thread.id(), now, privilege, positive, negative);

    for (auto tag : thread.tags())
    {
        if (tag)
        {
            updateDiscussionThreadPrivilege(userId, tag->id(), now, privilege, positive, negative);
        }
    }

    updateDiscussionThreadPrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadPrivilege(privilege));
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

    typedef std::tuple<DiscussionThreadMessagePrivilege, PrivilegeValueType, PrivilegeValueType, PrivilegeValueType, bool*> ThreadValuesTuple;
    ThreadValuesTuple threadValues[] =
    {
        ThreadValuesTuple{ DiscussionThreadMessagePrivilege::VIEW, {}, {}, {}, {} },
        ThreadValuesTuple{ DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER, {}, {}, {}, {} },
        ThreadValuesTuple{ DiscussionThreadMessagePrivilege::VIEW_VOTES, {}, {}, {}, {} },
        ThreadValuesTuple{ DiscussionThreadMessagePrivilege::VIEW_IP_ADDRESS, {}, {}, {}, {} },
    };

    auto& userId = items[0].userId;
    auto& firstMessage = *items[0].message;

    DiscussionThreadConstPtr thread = firstMessage.parentThread();
    assert(thread);
    //predetermine the privilege values granted and required at thread level as they are the same for all messages
    for (auto& tuple : threadValues)
    {
        auto privilege = std::get<0>(tuple);
        auto& positive = std::get<1>(tuple);
        auto& negative = std::get<2>(tuple);
        updateDiscussionThreadMessagePrivilege(userId, *thread, now, privilege, positive, negative);

        std::get<3>(tuple) = thread->getDiscussionThreadMessagePrivilege(privilege);
    }

    for (size_t i = 0; i < nrOfItems; ++i)
    {
        auto& item = items[i];
        std::get<4>(threadValues[0]) = &item.allowedToShowMessage;
        std::get<4>(threadValues[1]) = &item.allowedToShowUser;
        std::get<4>(threadValues[2]) = &item.allowedToShowVotes;
        std::get<4>(threadValues[3]) = &item.allowedToShowIpAddress;

        for (auto& tuple : threadValues)
        {
            auto privilege = std::get<0>(tuple);
            auto threadLevelPositive = std::get<1>(tuple);
            auto threadLevelNegative = std::get<2>(tuple);
            auto threadLevelRequired = std::get<3>(tuple);
            auto boolToUpdate = std::get<4>(tuple);

            PrivilegeValueType messageLevelPositive, messageLevelNegative;

            if (( ! item.userId) || ( ! item.message))
            {
                continue;
            }

            updateDiscussionThreadMessagePrivilege(item.userId, item.message->id(), now, privilege,
                                                   messageLevelPositive, messageLevelNegative);

            auto positive = maximumPrivilegeValue(messageLevelPositive, threadLevelPositive);
            auto negative = minimumPrivilegeValue(messageLevelNegative, threadLevelNegative);

            *boolToUpdate = static_cast<bool>(
                    ::isAllowed(positive, negative, item.message->getDiscussionThreadMessagePrivilege(privilege, threadLevelRequired)));
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
