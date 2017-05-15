#include "AuthorizationGrantedPrivilegeStore.h"

#include <boost/range/iterator_range.hpp>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

void GrantedPrivilegeStore::grantDiscussionThreadMessagePrivilege(IdType userId, IdType entityId, 
                                                                  DiscussionThreadMessagePrivilege privilege, 
                                                                  PrivilegeValueIntType value, time_t expiresAt)
{
    discussionThreadMessageSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionThreadPrivilege(IdType userId, IdType entityId, 
                                                           DiscussionThreadPrivilege privilege, 
                                                           PrivilegeValueIntType value, time_t expiresAt)
{
    discussionThreadSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionTagPrivilege(IdType userId, IdType entityId, 
                                                        DiscussionTagPrivilege privilege, 
                                                        PrivilegeValueIntType value, time_t expiresAt)
{
    discussionTagSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantDiscussionCategoryPrivilege(IdType userId, IdType entityId, 
                                                             DiscussionCategoryPrivilege privilege, 
                                                             PrivilegeValueIntType value, time_t expiresAt)
{
    discussionCategorySpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

void GrantedPrivilegeStore::grantForumWidePrivilege(IdType userId, IdType entityId, 
                                                    ForumWidePrivilege privilege, 
                                                    PrivilegeValueIntType value, time_t expiresAt)
{
    forumWideSpecificPrivileges_.insert(
            PrivilegeEntry(userId, entityId, static_cast<EnumIntType>(privilege), value, expiresAt));
}

static PrivilegeValueIntType getEffectivePrivilegeValue(PrivilegeValueType positive, PrivilegeValueType negative)
{
    return optionalOrZero(positive) - optionalOrZero(negative);
}

void GrantedPrivilegeStore::updateDiscussionThreadMessagePrivilege(const Entities::User& user, 
                                                                   const Entities::DiscussionThread& thread, time_t now, 
                                                                   DiscussionThreadMessagePrivilege privilege, 
                                                                   PrivilegeValueType& positiveValue, 
                                                                   PrivilegeValueType& negativeValue) const
{
    updateDiscussionThreadMessagePrivilege(user.id(), thread.id(), now, privilege, positiveValue, negativeValue);
    for (auto& tag : thread.tags())
    {
        if (tag)
        {
            updateDiscussionThreadMessagePrivilege(user.id(), tag->id(), now, privilege, positiveValue, negativeValue);
        }
    }

    updateDiscussionThreadMessagePrivilege(user.id(), {}, now, privilege, positiveValue, negativeValue);
}

static PrivilegeValueType isAllowed(PrivilegeValueType positive, PrivilegeValueType negative, PrivilegeValueType required)
{
    auto effectivePrivilegeValue = getEffectivePrivilegeValue(positive, negative);
    auto requiredPrivilegeValue = optionalOrZero(required);

    return (effectivePrivilegeValue >= requiredPrivilegeValue) ? effectivePrivilegeValue : PrivilegeValueType{};
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(const User& user, const DiscussionThreadMessage& message,
                                                    DiscussionThreadMessagePrivilege privilege, time_t now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionThreadMessagePrivilege(user.id(), message.id(), now, privilege, positive, negative);

    message.executeActionWithParentThreadIfAvailable([this, &positive, &negative, &user, privilege, now](auto& thread)
    {
        updateDiscussionThreadMessagePrivilege(user, thread, now, privilege, positive, negative);
    });

    return ::isAllowed(positive, negative, message.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(const User& user, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege, time_t now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionThreadPrivilege(user.id(), thread.id(), now, privilege, positive, negative);

    for (auto& tag : thread.tags())
    {
        if (tag)
        {
            updateDiscussionThreadPrivilege(user.id(), tag->id(), now, privilege, positive, negative);
        }
    }

    updateDiscussionThreadPrivilege(user.id(), {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, thread.getDiscussionThreadPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(const User& user, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege, time_t now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionTagPrivilege(user.id(), tag.id(), now, privilege, positive, negative);

    updateDiscussionTagPrivilege(user.id(), {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, tag.getDiscussionTagPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(const User& user, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege, time_t now) const
{
    PrivilegeValueType positive, negative;
    updateDiscussionCategoryPrivilege(user.id(), category.id(), now, privilege, positive, negative);

    updateDiscussionCategoryPrivilege(user.id(), {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, category.getDiscussionCategoryPrivilege(privilege));
}

PrivilegeValueType GrantedPrivilegeStore::isAllowed(IdType userId, const ForumWidePrivilegeStore& forumWidePrivilegeStore,
                                                    ForumWidePrivilege privilege, time_t now) const
{
    PrivilegeValueType positive, negative;

    updateForumWidePrivilege(userId, {}, now, privilege, positive, negative);

    return ::isAllowed(positive, negative, forumWidePrivilegeStore.getForumWidePrivilege(privilege));
}

void GrantedPrivilegeStore::computeDiscussionThreadMessageVisibilityAllowed(DiscussionThreadMessagePrivilegeCheck* items, 
                                                                            size_t nrOfItems, time_t now) const
{
    if (nrOfItems < 1)
    {
        return;
    }

    std::tuple<DiscussionThreadMessagePrivilege, PrivilegeValueType, PrivilegeValueType, PrivilegeValueType, bool*> threadValues[] =
    {
        { DiscussionThreadMessagePrivilege::VIEW, {}, {}, {}, {} },
        { DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER, {}, {}, {}, {} },
        { DiscussionThreadMessagePrivilege::VIEW_VOTES, {}, {}, {}, {} },
        { DiscussionThreadMessagePrivilege::VIEW_IP_ADDRESS, {}, {}, {}, {} },
    };

    auto& user = *items[0].user;
    auto& firstMessage = *items[0].message;

    firstMessage.executeActionWithParentThreadIfAvailable([this, &user, now, &threadValues](auto& thread)
    {
        //predetermine the privilege values granted and required at thread level as they are the same for all messages
        for (auto& tuple : threadValues)
        {
            auto privilege = std::get<0>(tuple);
            auto& positive = std::get<1>(tuple);
            auto& negative = std::get<2>(tuple);
            updateDiscussionThreadMessagePrivilege(user, thread, now, privilege, positive, negative);

            std::get<3>(tuple) = thread.getDiscussionThreadMessagePrivilege(privilege);
        }
    });

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
            
            if (( ! item.user) || ( ! item.message))
            {
                continue;
            }

            updateDiscussionThreadMessagePrivilege(item.user->id(), item.message->id(), now, privilege, 
                                                   messageLevelPositive, messageLevelNegative);
            
            auto positive = maximumPrivilegeValue(messageLevelPositive, threadLevelPositive);
            auto negative = minimumPrivilegeValue(messageLevelNegative, threadLevelNegative);

            *boolToUpdate = static_cast<bool>(
                    ::isAllowed(positive, negative, item.message->getDiscussionThreadMessagePrivilege(privilege, threadLevelRequired)));
        }
    }
}

void GrantedPrivilegeStore::updateDiscussionThreadMessagePrivilege(IdType userId, IdType entityId, time_t now, 
                                                                   DiscussionThreadMessagePrivilege privilege,
                                                                   PrivilegeValueType& positiveValue,
                                                                   PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionThreadMessageSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege), 
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateDiscussionThreadPrivilege(IdType userId, IdType entityId, time_t now, 
                                                            DiscussionThreadPrivilege privilege,
                                                            PrivilegeValueType& positiveValue, 
                                                            PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionThreadSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege), 
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateDiscussionTagPrivilege(IdType userId, IdType entityId, time_t now, 
                                                         DiscussionTagPrivilege privilege,
                                                         PrivilegeValueType& positiveValue, 
                                                         PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionTagSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege), 
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateDiscussionCategoryPrivilege(IdType userId, IdType entityId, time_t now, 
                                                              DiscussionCategoryPrivilege privilege,
                                                              PrivilegeValueType& positiveValue,
                                                              PrivilegeValueType& negativeValue) const
{
    updatePrivilege(discussionCategorySpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege), 
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updateForumWidePrivilege(IdType userId, IdType entityId, time_t now, 
                                                     ForumWidePrivilege privilege,
                                                     PrivilegeValueType& positiveValue, 
                                                     PrivilegeValueType& negativeValue) const
{
    updatePrivilege(forumWideSpecificPrivileges_, userId, entityId, now, static_cast<EnumIntType>(privilege), 
                    positiveValue, negativeValue);
}

void GrantedPrivilegeStore::updatePrivilege(const PrivilegeEntryCollection& collection, IdType userId, IdType entityId,
                                            time_t now, EnumIntType privilege, PrivilegeValueType& positiveValue, 
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
