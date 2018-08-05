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

#include "MemoryRepositoryAuthorization.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "ContextProviders.h"
#include "Logging.h"

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

using namespace Json;


MemoryRepositoryAuthorization::MemoryRepositoryAuthorization(MemoryStoreRef store,
                                                             ForumWideAuthorizationRef forumWideAuthorization,
                                                             DiscussionThreadAuthorizationRef threadAuthorization,
                                                             DiscussionThreadMessageAuthorizationRef threadMessageAuthorization,
                                                             DiscussionTagAuthorizationRef tagAuthorization,
                                                             DiscussionCategoryAuthorizationRef categoryAuthorization)
    : MemoryRepositoryBase(std::move(store)), forumWideAuthorization_(forumWideAuthorization),
      threadAuthorization_(threadAuthorization), threadMessageAuthorization_(threadMessageAuthorization),
      tagAuthorization_(tagAuthorization), categoryAuthorization_(categoryAuthorization)
{
    if ( ! forumWideAuthorization_ || ! threadAuthorization_ || ! threadMessageAuthorization_ || ! tagAuthorization_
        || ! categoryAuthorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

//
//common
//

template<typename Store, typename Enum, typename Fn>
void writePrivilegeValues(const Store& store, Enum maxValue, JsonWriter& writer, const StringView* strings, Fn fn)
{
    writer.startArray();

    for (EnumIntType i = 0; i < static_cast<EnumIntType>(maxValue); ++i)
    {
        auto privilege = static_cast<Enum>(i);
        auto value = fn(store, privilege);
        if (value)
        {
            writer.startObject();
            writer.newPropertyWithSafeName("name") << strings[i];
            writer.newPropertyWithSafeName("value") << *value;
            writer.endObject();
        }
    }

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionThreadMessageRequiredPrivileges(
        const DiscussionThreadMessagePrivilegeStore& store, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionThreadMessagePrivileges");
    writePrivilegeValues(store, DiscussionThreadMessagePrivilege::COUNT, writer, DiscussionThreadMessagePrivilegeStrings,
                        [](const DiscussionThreadMessagePrivilegeStore& store, DiscussionThreadMessagePrivilege privilege)
                        {
                            return store.DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
                        });
}

void MemoryRepositoryAuthorization::writeDiscussionThreadRequiredPrivileges(const DiscussionThreadPrivilegeStore& store,
                                                                            JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionThreadPrivileges");
    writePrivilegeValues(store, DiscussionThreadPrivilege::COUNT, writer, DiscussionThreadPrivilegeStrings,
                         [](const DiscussionThreadPrivilegeStore& store, DiscussionThreadPrivilege privilege)
                         {
                             return store.DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);
                         });
}

void MemoryRepositoryAuthorization::writeDiscussionTagRequiredPrivileges(const DiscussionTagPrivilegeStore& store,
                                                                         JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionTagPrivileges");
    writePrivilegeValues(store, DiscussionTagPrivilege::COUNT, writer, DiscussionTagPrivilegeStrings,
                        [](const DiscussionTagPrivilegeStore& store, DiscussionTagPrivilege privilege)
                        {
                            return store.DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege);
                        });
}

void MemoryRepositoryAuthorization::writeDiscussionCategoryRequiredPrivileges(const DiscussionCategoryPrivilegeStore& store,
                                                                              JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionCategoryPrivileges");
    writePrivilegeValues(store, DiscussionCategoryPrivilege::COUNT, writer, DiscussionCategoryPrivilegeStrings,
                        [](const DiscussionCategoryPrivilegeStore& store, DiscussionCategoryPrivilege privilege)
                        {
                            return store.DiscussionCategoryPrivilegeStore::getDiscussionCategoryPrivilege(privilege);
                        });
}

void MemoryRepositoryAuthorization::writeForumWideRequiredPrivileges(const ForumWidePrivilegeStore& store,
                                                                     JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forumWidePrivileges");
    writePrivilegeValues(store, ForumWidePrivilege::COUNT, writer, ForumWidePrivilegeStrings,
                        [](const ForumWidePrivilegeStore& store, ForumWidePrivilege privilege)
                        {
                            return store.getForumWidePrivilege(privilege);
                        });
}

void MemoryRepositoryAuthorization::writeForumWideDefaultPrivilegeLevels(const ForumWidePrivilegeStore& store,
                                                                         JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forumWideDefaultLevels");
    writer.startArray();

    for (EnumIntType i = 0; i < static_cast<EnumIntType>(ForumWideDefaultPrivilegeDuration::COUNT); ++i)
    {
        const auto privilege = static_cast<ForumWideDefaultPrivilegeDuration>(i);
        auto value = store.getForumWideDefaultPrivilegeLevel(privilege);
        if (value)
        {
            writer.startObject();
            writer.newPropertyWithSafeName("name") << ForumWideDefaultPrivilegeDurationStrings[i];
            writer.newPropertyWithSafeName("value") << value->value;
            writer.newPropertyWithSafeName("duration") << value->duration;
            writer.endObject();
        }
    }
    writer.endArray();
}

struct AssignedPrivilegeWriter final
{
    AssignedPrivilegeWriter(JsonWriter& writer, const EntityCollection& collection)
            : writer_(writer), collection_(collection)
    {
    }

    AssignedPrivilegeWriter(const AssignedPrivilegeWriter&) = default;
    AssignedPrivilegeWriter(AssignedPrivilegeWriter&&) = default;
    
    void operator()(IdTypeRef userId, const PrivilegeValueIntType privilegeValue, const Timestamp grantedAt, 
                    const Timestamp expiresAt)
    {
        writer_.startObject();
        writer_.newPropertyWithSafeName("id") << userId;

        if (userId == anonymousUserId())
        {
            writer_.newPropertyWithSafeName("name") << anonymousUser()->name();
        }
        else
        {
            const auto& index = collection_.users().byId();
            const auto it = index.find(userId);
            if (it != index.end())
            {
                writer_.newPropertyWithSafeName("name") << (**it).name();
            }
        }

        writer_.newPropertyWithSafeName("value") << privilegeValue;
        writer_.newPropertyWithSafeName("granted") << grantedAt;
        writer_.newPropertyWithSafeName("expires") << expiresAt;
        writer_.endObject();
    }
private:
    JsonWriter& writer_;
    const EntityCollection& collection_;
};

struct UserAssignedPrivilegeWriter final
{
    typedef bool(*WriteEntityFunction)(const EntityCollection& collection, const SerializationRestriction& restriction,
                                     IdTypeRef entityId, JsonWriter& writer);

    UserAssignedPrivilegeWriter(JsonWriter& writer, const EntityCollection& collection, 
        const SerializationRestriction& restriction, WriteEntityFunction writeEntity)
            : writer_(writer), collection_(collection), restriction_(restriction), writeEntity_(writeEntity) {}
    ~UserAssignedPrivilegeWriter() = default;

    UserAssignedPrivilegeWriter(const UserAssignedPrivilegeWriter&) = default;
    UserAssignedPrivilegeWriter(UserAssignedPrivilegeWriter&&) = default;

    UserAssignedPrivilegeWriter& operator=(const UserAssignedPrivilegeWriter&) = delete;
    UserAssignedPrivilegeWriter& operator=(UserAssignedPrivilegeWriter&&) = delete;

    void operator()(IdTypeRef entityId, PrivilegeValueIntType const privilegeValue, const Timestamp grantedAt, 
                    const Timestamp expiresAt)
    {
        writer_.startObject();
        if (writeEntity_(collection_, restriction_, entityId, writer_))
        {
            writer_.newPropertyWithSafeName("value") << privilegeValue;
            writer_.newPropertyWithSafeName("granted") << grantedAt;
            writer_.newPropertyWithSafeName("expires") << expiresAt;
        }
        writer_.endObject();
    }
private:
    JsonWriter& writer_;
    const EntityCollection& collection_;
    const SerializationRestriction& restriction_;
    WriteEntityFunction writeEntity_;
};

static bool writeDiscussionThreadEntity(const EntityCollection& collection, const SerializationRestriction& restriction, 
                                        IdTypeRef entityId, JsonWriter& writer)
{
    const auto threadPtr = collection.threads().findById(entityId);
    if (threadPtr)
    {
        const DiscussionThread& thread = *threadPtr;
        if (restriction.isAllowed(thread, DiscussionThreadPrivilege::VIEW))
        {
            writer.newPropertyWithSafeName("id") << thread.id();
            writer.newPropertyWithSafeName("name") << thread.name();
            return true;
        }
    }
    return false;
}

static bool writeDiscussionTagEntity(const EntityCollection& collection, const SerializationRestriction& restriction, 
                                     IdTypeRef entityId, JsonWriter& writer)
{
    const auto& index = collection.tags().byId();
    const auto it = index.find(entityId);
    if (it != index.end())
    {
        const DiscussionTag& tag = **it;
        if (restriction.isAllowed(tag, DiscussionTagPrivilege::VIEW))
        {
            writer.newPropertyWithSafeName("id") << tag.id();
            writer.newPropertyWithSafeName("name") << tag.name();
            return true;
        }
    }
    return false;
}

static bool writeDiscussionCategoryEntity(const EntityCollection& collection, const SerializationRestriction& restriction,
                                          IdTypeRef entityId, JsonWriter& writer)
{
    const auto& index = collection.categories().byId();
    const auto it = index.find(entityId);
    if (it != index.end())
    {
        const DiscussionCategory& category = **it;
        if (restriction.isAllowed(category, DiscussionCategoryPrivilege::VIEW))
        {
            writer.newPropertyWithSafeName("id") << category.id();
            writer.newPropertyWithSafeName("name") << category.name();
            return true;
        }
    }
    return false;
}

static bool writeForumWideEntity(const EntityCollection& collection, const SerializationRestriction& restriction, 
                                 IdTypeRef entityId, JsonWriter& writer)
{
    //do nothing
    return true;
}

void writeAssignedPrivilegesExtra(JsonWriter& writer)
{
    writer.newPropertyWithSafeName("now") << Context::getCurrentTime();
}

void MemoryRepositoryAuthorization::writeDiscussionThreadMessageAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                                   JsonWriter& writer)
{
    writeAssignedPrivilegesExtra(writer);

    writer.newPropertyWithSafeName("discussionThreadMessagePrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionThreadMessagePrivileges(id,
            AssignedPrivilegeWriter(writer, collection));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionThreadAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                            JsonWriter& writer)
{
    writeAssignedPrivilegesExtra(writer);

    writer.newPropertyWithSafeName("discussionThreadPrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionThreadPrivileges(id, AssignedPrivilegeWriter(writer, collection));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionTagAssignedPrivileges(const EntityCollection& collection,
                                                                         IdTypeRef id, JsonWriter& writer)
{
    writeAssignedPrivilegesExtra(writer);

    writer.newPropertyWithSafeName("discussionTagPrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionTagPrivileges(id, AssignedPrivilegeWriter(writer, collection));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionCategoryAssignedPrivileges(const EntityCollection& collection,
                                                                              IdTypeRef id, JsonWriter& writer)
{
    writeAssignedPrivilegesExtra(writer);

    writer.newPropertyWithSafeName("discussionCategoryPrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionCategoryPrivileges(id, AssignedPrivilegeWriter(writer, collection));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeForumWideAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                     JsonWriter& writer)
{
    writeAssignedPrivilegesExtra(writer);

    writer.newPropertyWithSafeName("forumWidePrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateForumWidePrivileges(id, AssignedPrivilegeWriter(writer, collection));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionThreadUserAssignedPrivileges(const EntityCollection& collection,
                                                                                const SerializationRestriction& restriction,
                                                                                IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionThreadPrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionThreadPrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, restriction, writeDiscussionThreadEntity));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionTagUserAssignedPrivileges(const EntityCollection& collection,
                                                                             const SerializationRestriction& restriction,
                                                                             IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionTagPrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionTagPrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, restriction, writeDiscussionTagEntity));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionCategoryUserAssignedPrivileges(const EntityCollection& collection,
                                                                                  const SerializationRestriction& restriction,
                                                                                  IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussionCategoryPrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionCategoryPrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, restriction, writeDiscussionCategoryEntity));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeForumWideUserAssignedPrivileges(const EntityCollection& collection,
                                                                         const SerializationRestriction& restriction,
                                                                         IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forumWidePrivileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateForumWidePrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, restriction, writeForumWideEntity));

    writer.endArray();
}

static void writeAllowPrivilegeChange(const IDiscussionThreadMessageAuthorization& authorization, 
                                      const DiscussionThreadMessage& threadMessage, const User& currentUser, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("allowAdjustPrivilege") <<
        (AuthorizationStatus::OK == authorization.getAllowDiscussionThreadMessagePrivilegeChange(currentUser, threadMessage));
}

static void writeAllowPrivilegeChange(const IDiscussionThreadAuthorization& authorization, 
                                      const DiscussionThread& thread, const User& currentUser, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("allowAdjustPrivilege") <<
        (AuthorizationStatus::OK == authorization.getAllowDiscussionThreadPrivilegeChange(currentUser, thread));
}

static void writeAllowPrivilegeChange(const IDiscussionTagAuthorization& authorization, 
                                      const DiscussionTag& tag, const User& currentUser, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("allowAdjustPrivilege") <<
        (AuthorizationStatus::OK == authorization.getAllowDiscussionTagPrivilegeChange(currentUser, tag));
}

static void writeAllowPrivilegeChange(const IDiscussionCategoryAuthorization& authorization, 
                                      const DiscussionCategory& category, const User& currentUser, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("allowAdjustPrivilege") <<
        (AuthorizationStatus::OK == authorization.getAllowDiscussionCategoryPrivilegeChange(currentUser, category));
}

static void writeAllowPrivilegeChange(const IForumWideAuthorization& authorization, 
                                      const User& currentUser, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("allowAdjustPrivilege") <<
        (AuthorizationStatus::OK == authorization.getAllowForumWidePrivilegeChange(currentUser));
}

//
//discussion thread message
//

StatusCode MemoryRepositoryAuthorization::getRequiredPrivilegesForThreadMessage(IdTypeRef messageId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.threadMessages().byId();
                          auto it = index.find(messageId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& message = **it;

                          if ( ! (status = threadMessageAuthorization_->getDiscussionThreadMessageRequiredPrivileges(currentUser, message)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadMessageRequiredPrivileges(message, writer);
                          writeAllowPrivilegeChange(*threadMessageAuthorization_, message, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetRequiredPrivilegesFromThreadMessage(createObserverContext(currentUser),
                                                                                message);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivilegesForThreadMessage(IdTypeRef messageId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.threadMessages().byId();
                          auto it = index.find(messageId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& message = **it;

                          if ( ! (status = threadMessageAuthorization_->getDiscussionThreadMessageAssignedPrivileges(currentUser, message)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadMessageAssignedPrivileges(collection, message.id(), writer);
                          writeAllowPrivilegeChange(*threadMessageAuthorization_, message, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetAssignedPrivilegesFromThreadMessage(createObserverContext(currentUser),
                                                                                message);
                      });
    return status;
}

template<typename EnumType>
bool isValidPrivilege(EnumType value)
{
    const auto intValue = static_cast<EnumIntType>(value);
    return intValue >= 0 && intValue < static_cast<EnumIntType>(EnumType::COUNT);
}

static bool isValidPrivilegeValue(PrivilegeValueIntType value)
{
    return value >= MinPrivilegeValue && value <= MaxPrivilegeValue;
}

static bool isValidPrivilegeDuration(PrivilegeDurationIntType value)
{
    return value >= 0;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
        IdTypeRef messageId, DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! messageId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threadMessages().byId();
                           auto it = indexById.find(messageId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionThreadMessage& message = **it;
                           auto oldValue = message.DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

                           if ( ! (status = threadMessageAuthorization_->updateDiscussionThreadMessagePrivilege(
                                   *currentUser, message, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
                                   createObserverContext(*currentUser), message, privilege, value);

                           status = changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
                                   collection, messageId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
        EntityCollection& collection, IdTypeRef messageId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value)
{
    if ( ! messageId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.threadMessages().byId();
    const auto it = indexById.find(messageId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(messageId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionThreadMessagePtr message = *it;
    message->setDiscussionThreadMessagePrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilege(
        IdTypeRef messageId, IdTypeRef userId, PrivilegeValueIntType value, PrivilegeDurationIntType duration,
        OutStream& output)
{
    StatusWriter status(output);
    if ( ! messageId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& messageIndexById = collection.threadMessages().byId();
                           auto messageIt = messageIndexById.find(messageId);
                           if (messageIt == messageIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           const DiscussionThreadMessage& message = **messageIt;

                           auto targetUser = anonymousUser();
                           if (userId != anonymousUserId())
                           {
                               auto& userIndexById = collection.users().byId();
                               auto userIt = userIndexById.find(userId);
                               if (userIt == userIndexById.end())
                               {
                                   status = StatusCode::NOT_FOUND;
                                   return;
                               }
                               targetUser = *userIt;
                           }

                           if ( ! (status = threadMessageAuthorization_->assignDiscussionThreadMessagePrivilege(
                                   *currentUser, message, *targetUser, value)))
                           {
                               return;
                           }

                           writeEvents().onAssignDiscussionThreadMessagePrivilege(
                                   createObserverContext(*currentUser), message, *targetUser, value, duration);

                           status = assignDiscussionThreadMessagePrivilege(
                                   collection, messageId, userId, value, duration);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilege(
        EntityCollection& collection, IdTypeRef messageId, IdTypeRef userId, PrivilegeValueIntType value,
        PrivilegeDurationIntType duration)
{
    if ( ! messageId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.threadMessages().byId();
    const auto it = indexById.find(messageId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(messageId);
        return StatusCode::NOT_FOUND;
    }

    if (userId != anonymousUserId())
    {
        auto& userIndexById = collection.users().byId();
        const auto userIt = userIndexById.find(userId);
        if (userIt == userIndexById.end())
        {
            FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(userId);
            return StatusCode::NOT_FOUND;
        }
    }

    const auto now = Context::getCurrentTime();
    const auto expiresAt = calculatePrivilegeExpires(now, duration);
    collection.grantedPrivileges().grantDiscussionThreadMessagePrivilege(userId, messageId, value, now, expiresAt);

    return StatusCode::OK;
}

//
//discussion thread
//

StatusCode MemoryRepositoryAuthorization::getRequiredPrivilegesForThread(IdTypeRef threadId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto threadPtr = collection.threads().findById(threadId);
                          if ( ! threadPtr)
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          const auto& thread = *threadPtr;

                          if ( ! (status = threadAuthorization_->getDiscussionThreadRequiredPrivileges(currentUser, thread)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadRequiredPrivileges(thread, writer);
                          writeDiscussionThreadMessageRequiredPrivileges(thread, writer);
                          writeAllowPrivilegeChange(*threadAuthorization_, thread, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetRequiredPrivilegesFromThread(createObserverContext(currentUser), thread);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivilegesForThread(IdTypeRef threadId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto threadPtr = collection.threads().findById(threadId);
                          if ( ! threadPtr)
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          const auto& thread = *threadPtr;

                          if ( ! (status = threadAuthorization_->getDiscussionThreadAssignedPrivileges(currentUser, thread)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadAssignedPrivileges(collection, thread.id(), writer);
                          writeAllowPrivilegeChange(*threadAuthorization_, thread, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetAssignedPrivilegesFromThread(createObserverContext(currentUser), thread);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilegeForThread(
        IdTypeRef threadId, DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! threadId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           const auto threadPtr = collection.threads().findById(threadId);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionThread& thread = *threadPtr;
                           auto oldValue = thread.DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

                           if ( ! (status = threadAuthorization_->updateDiscussionThreadMessagePrivilege(
                                   *currentUser, thread, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadMessageRequiredPrivilegeForThread(
                                   createObserverContext(*currentUser), thread, privilege, value);

                           status = changeDiscussionThreadMessageRequiredPrivilegeForThread(
                                   collection, threadId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilegeForThread(
        EntityCollection& collection, IdTypeRef threadId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value)
{
    if ( ! threadId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto threadPtr = collection.threads().findById(threadId);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(threadId);
        return StatusCode::NOT_FOUND;
    }

    threadPtr->setDiscussionThreadMessagePrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadRequiredPrivilegeForThread(
        IdTypeRef threadId, DiscussionThreadPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! threadId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           const auto threadPtr = collection.threads().findById(threadId);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionThread& thread = *threadPtr;
                           auto oldValue = thread.DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);

                           if ( ! (status = threadAuthorization_->updateDiscussionThreadPrivilege(
                                   *currentUser, thread, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadRequiredPrivilegeForThread(
                                   createObserverContext(*currentUser), thread, privilege, value);

                           status = changeDiscussionThreadRequiredPrivilegeForThread(
                                   collection, threadId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadRequiredPrivilegeForThread(
        EntityCollection& collection, IdTypeRef threadId, DiscussionThreadPrivilege privilege,
        PrivilegeValueIntType value)
{
    if ( ! threadId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto threadPtr = collection.threads().findById(threadId);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(threadId);
        return StatusCode::NOT_FOUND;
    }

    threadPtr->setDiscussionThreadPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilege(
        IdTypeRef threadId, IdTypeRef userId, PrivilegeValueIntType value, PrivilegeDurationIntType duration,
        OutStream& output)
{
    StatusWriter status(output);
    if ( ! threadId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           const auto threadPtr = collection.threads().findById(threadId);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           const DiscussionThread& thread = *threadPtr;

                           auto targetUser = anonymousUser();
                           if (userId != anonymousUserId())
                           {
                               auto& userIndexById = collection.users().byId();
                               auto userIt = userIndexById.find(userId);
                               if (userIt == userIndexById.end())
                               {
                                   status = StatusCode::NOT_FOUND;
                                   return;
                               }
                               targetUser = *userIt;
                           }

                           if ( ! (status = threadAuthorization_->assignDiscussionThreadPrivilege(
                                   *currentUser, thread, *targetUser, value)))
                           {
                               return;
                           }

                           writeEvents().onAssignDiscussionThreadPrivilege(
                                   createObserverContext(*currentUser), thread, *targetUser, value, duration);

                           status = assignDiscussionThreadPrivilege(collection, threadId, userId, value, duration);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilege(
        EntityCollection& collection, IdTypeRef threadId, IdTypeRef userId, PrivilegeValueIntType value,
        PrivilegeDurationIntType duration)
{
    if ( ! threadId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    if ( ! collection.threads().findById(threadId))
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(threadId);
        return StatusCode::NOT_FOUND;
    }

    if (userId != anonymousUserId())
    {
        auto& userIndexById = collection.users().byId();
        const auto userIt = userIndexById.find(userId);
        if (userIt == userIndexById.end())
        {
            FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(userId);
            return StatusCode::NOT_FOUND;
        }
    }

    const auto now = Context::getCurrentTime();
    const auto expiresAt = calculatePrivilegeExpires(now, duration);
    collection.grantedPrivileges().grantDiscussionThreadPrivilege(userId, threadId, value, now, expiresAt);

    return StatusCode::OK;
}

//
//discussion tag
//

StatusCode MemoryRepositoryAuthorization::getRequiredPrivilegesForTag(IdTypeRef tagId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.tags().byId();
                          auto it = index.find(tagId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& tag = **it;

                          if ( ! (status = tagAuthorization_->getDiscussionTagRequiredPrivileges(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionTagRequiredPrivileges(tag, writer);
                          writeDiscussionThreadRequiredPrivileges(tag, writer);
                          writeDiscussionThreadMessageRequiredPrivileges(tag, writer);
                          writeAllowPrivilegeChange(*tagAuthorization_, tag, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetRequiredPrivilegesFromTag(createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivilegesForTag(IdTypeRef tagId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.tags().byId();
                          auto it = index.find(tagId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& tag = **it;

                          if ( ! (status = tagAuthorization_->getDiscussionTagAssignedPrivileges(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionTagAssignedPrivileges(collection, tag.id(), writer);
                          writeAllowPrivilegeChange(*tagAuthorization_, tag, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetAssignedPrivilegesFromTag(createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilegeForTag(
        IdTypeRef tagId, DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(tagId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionTag& tag = **it;
                           auto oldValue = tag.DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

                           if ( ! (status = tagAuthorization_->updateDiscussionThreadMessagePrivilege(
                                   *currentUser, tag, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadMessageRequiredPrivilegeForTag(
                                   createObserverContext(*currentUser), tag, privilege, value);

                           status = changeDiscussionThreadMessageRequiredPrivilegeForTag(
                                   collection, tagId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilegeForTag(
        EntityCollection& collection, IdTypeRef tagId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value)
{
    if ( ! tagId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.tags().byId();
    auto it = indexById.find(tagId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << static_cast<std::string>(tagId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tag = *it;
    tag->setDiscussionThreadMessagePrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadRequiredPrivilegeForTag(
        IdTypeRef tagId, DiscussionThreadPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(tagId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionTag& tag = **it;
                           auto oldValue = tag.DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);

                           if ( ! (status = tagAuthorization_->updateDiscussionThreadPrivilege(
                                   *currentUser, tag, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadRequiredPrivilegeForTag(
                                   createObserverContext(*currentUser), tag, privilege, value);

                           status = changeDiscussionThreadRequiredPrivilegeForTag(
                                   collection, tagId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadRequiredPrivilegeForTag(
        EntityCollection& collection, IdTypeRef tagId, DiscussionThreadPrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! tagId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.tags().byId();
    auto it = indexById.find(tagId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << static_cast<std::string>(tagId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tag = *it;
    tag->setDiscussionThreadPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionTagRequiredPrivilegeForTag(
        IdTypeRef tagId, DiscussionTagPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(tagId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionTag& tag = **it;
                           auto oldValue = tag.DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege);

                           if ( ! (status = tagAuthorization_->updateDiscussionTagPrivilege(
                                   *currentUser, tag, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionTagRequiredPrivilegeForTag(
                                   createObserverContext(*currentUser), tag, privilege, value);

                           status = changeDiscussionTagRequiredPrivilegeForTag(
                                   collection, tagId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionTagRequiredPrivilegeForTag(
        EntityCollection& collection, IdTypeRef tagId, DiscussionTagPrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! tagId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.tags().byId();
    auto it = indexById.find(tagId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << static_cast<std::string>(tagId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tag = *it;
    tag->setDiscussionTagPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionTagPrivilege(
        IdTypeRef tagId, IdTypeRef userId, PrivilegeValueIntType value, PrivilegeDurationIntType duration,
        OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           const DiscussionTag& tag = **tagIt;

                           auto targetUser = anonymousUser();
                           if (userId != anonymousUserId())
                           {
                               auto& userIndexById = collection.users().byId();
                               auto userIt = userIndexById.find(userId);
                               if (userIt == userIndexById.end())
                               {
                                   status = StatusCode::NOT_FOUND;
                                   return;
                               }
                               targetUser = *userIt;
                           }

                           if ( ! (status = tagAuthorization_->assignDiscussionTagPrivilege(
                                   *currentUser, tag, *targetUser, value)))
                           {
                               return;
                           }

                           writeEvents().onAssignDiscussionTagPrivilege(
                                   createObserverContext(*currentUser), tag, *targetUser, value, duration);

                           status = assignDiscussionTagPrivilege(collection, tagId, userId, value, duration);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionTagPrivilege(
        EntityCollection& collection, IdTypeRef tagId, IdTypeRef userId, PrivilegeValueIntType value,
        PrivilegeDurationIntType duration)
{
    if ( ! tagId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.tags().byId();
    auto it = indexById.find(tagId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << static_cast<std::string>(tagId);
        return StatusCode::NOT_FOUND;
    }

    if (userId != anonymousUserId())
    {
        auto& userIndexById = collection.users().byId();
        auto userIt = userIndexById.find(userId);
        if (userIt == userIndexById.end())
        {
            FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(userId);
            return StatusCode::NOT_FOUND;
        }
    }

    auto now = Context::getCurrentTime();
    auto expiresAt = calculatePrivilegeExpires(now, duration);
    collection.grantedPrivileges().grantDiscussionTagPrivilege(userId, tagId, value, now, expiresAt);

    return StatusCode::OK;
}

//
//discussion category
//

StatusCode MemoryRepositoryAuthorization::getRequiredPrivilegesForCategory(IdTypeRef categoryId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.categories().byId();
                          auto it = index.find(categoryId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& category = **it;

                          if ( ! (status = categoryAuthorization_->getDiscussionCategoryRequiredPrivileges(currentUser, category)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionCategoryRequiredPrivileges(category, writer);
                          writeAllowPrivilegeChange(*categoryAuthorization_, category, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetRequiredPrivilegesFromCategory(createObserverContext(currentUser), category);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivilegesForCategory(IdTypeRef categoryId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.categories().byId();
                          auto it = index.find(categoryId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& category = **it;

                          if ( ! (status = categoryAuthorization_->getDiscussionCategoryAssignedPrivileges(currentUser, category)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionCategoryAssignedPrivileges(collection, category.id(), writer);
                          writeAllowPrivilegeChange(*categoryAuthorization_, category, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetAssignedPrivilegesFromCategory(createObserverContext(currentUser), category);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionCategoryRequiredPrivilegeForCategory(
        IdTypeRef categoryId, DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! categoryId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().byId();
                           auto it = indexById.find(categoryId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionCategory& category = **it;
                           auto oldValue = category.DiscussionCategoryPrivilegeStore::getDiscussionCategoryPrivilege(privilege);

                           if ( ! (status = categoryAuthorization_->updateDiscussionCategoryPrivilege(
                                   *currentUser, category, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionCategoryRequiredPrivilegeForCategory(
                                   createObserverContext(*currentUser), category, privilege, value);

                           status = changeDiscussionCategoryRequiredPrivilegeForCategory(
                                   collection, categoryId, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionCategoryRequiredPrivilegeForCategory(
        EntityCollection& collection, IdTypeRef categoryId, DiscussionCategoryPrivilege privilege,
        PrivilegeValueIntType value)
{
    if ( ! categoryId || ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.categories().byId();
    auto it = indexById.find(categoryId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << static_cast<std::string>(categoryId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionCategoryPtr category = *it;
    category->setDiscussionCategoryPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionCategoryPrivilege(
        IdTypeRef categoryId, IdTypeRef userId, PrivilegeValueIntType value, PrivilegeDurationIntType duration,
        OutStream& output)
{
    StatusWriter status(output);
    if ( ! categoryId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& categoryIndexById = collection.categories().byId();
                           auto categoryIt = categoryIndexById.find(categoryId);
                           if (categoryIt == categoryIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           const DiscussionCategory& category = **categoryIt;

                           auto targetUser = anonymousUser();
                           if (userId != anonymousUserId())
                           {
                               auto& userIndexById = collection.users().byId();
                               auto userIt = userIndexById.find(userId);
                               if (userIt == userIndexById.end())
                               {
                                   status = StatusCode::NOT_FOUND;
                                   return;
                               }
                               targetUser = *userIt;
                           }

                           if ( ! (status = categoryAuthorization_->assignDiscussionCategoryPrivilege(
                                   *currentUser, category, *targetUser, value)))
                           {
                               return;
                           }

                           writeEvents().onAssignDiscussionCategoryPrivilege(
                                   createObserverContext(*currentUser), category, *targetUser, value, duration);

                           status = assignDiscussionCategoryPrivilege(collection, categoryId, userId, value, duration);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionCategoryPrivilege(
        EntityCollection& collection, IdTypeRef categoryId, IdTypeRef userId, PrivilegeValueIntType value,
        PrivilegeDurationIntType duration)
{
    if ( ! categoryId || ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto& indexById = collection.categories().byId();
    auto it = indexById.find(categoryId);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << static_cast<std::string>(categoryId);
        return StatusCode::NOT_FOUND;
    }

    if (userId != anonymousUserId())
    {
        auto& userIndexById = collection.users().byId();
        auto userIt = userIndexById.find(userId);
        if (userIt == userIndexById.end())
        {
            FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(userId);
            return StatusCode::NOT_FOUND;
        }
    }

    auto now = Context::getCurrentTime();
    auto expiresAt = calculatePrivilegeExpires(now, duration);
    collection.grantedPrivileges().grantDiscussionCategoryPrivilege(userId, categoryId, value, now, expiresAt);

    return StatusCode::OK;
}

//
//forum wide
//


StatusCode MemoryRepositoryAuthorization::getForumWideCurrentUserPrivileges(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          status = StatusCode::OK;
                          status.disable();

                          auto& currentUser = performedBy.get(collection, *store_);

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

                          JsonWriter writer(output);
                          writer.startObject();

                          writePrivileges(writer, collection, ForumWidePrivilegesToSerialize,
                                          ForumWidePrivilegeStrings, restriction);

                          writer.endObject();

                          readEvents().onGetForumWideCurrentUserPrivileges(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getForumWideRequiredPrivileges(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = forumWideAuthorization_->getForumWideRequiredPrivileges(currentUser)))
                          {
                              return;
                          }
                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideRequiredPrivileges(collection, writer);
                          writeDiscussionCategoryRequiredPrivileges(collection, writer);
                          writeDiscussionTagRequiredPrivileges(collection, writer);
                          writeDiscussionThreadRequiredPrivileges(collection, writer);
                          writeDiscussionThreadMessageRequiredPrivileges(collection, writer);
                          writeAllowPrivilegeChange(*forumWideAuthorization_, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetForumWideRequiredPrivileges(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getForumWideDefaultPrivilegeLevels(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = forumWideAuthorization_->getForumWideRequiredPrivileges(currentUser)))
                          {
                              return;
                          }
                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideDefaultPrivilegeLevels(collection, writer);

                          writer.endObject();

                          readEvents().onGetForumWideDefaultPrivilegeLevels(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getForumWideAssignedPrivileges(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = forumWideAuthorization_->getForumWideAssignedPrivileges(currentUser)))
                          {
                              return;
                          }
                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideAssignedPrivileges(collection, {}, writer);
                          writeAllowPrivilegeChange(*forumWideAuthorization_, currentUser, writer);

                          writer.endObject();

                          readEvents().onGetForumWideAssignedPrivileges(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivilegesForUser(IdTypeRef userId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          const User* userPtr = anonymousUser();
                          if (userId != anonymousUserId())
                          {
                              const auto& indexById = collection.users().byId();
                              auto it = indexById.find(userId);
                              if (it == indexById.end())
                              {
                                  status = StatusCode::NOT_FOUND;
                                  return;
                              }
                              userPtr = *it;
                          }

                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = forumWideAuthorization_->getUserAssignedPrivileges(currentUser, *userPtr)))
                          {
                              return;
                          }
                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeAssignedPrivilegesExtra(writer);

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                              currentUser.id(), Context::getCurrentTime());

                          writeForumWideUserAssignedPrivileges(collection, restriction, userId, writer);
                          writeDiscussionCategoryUserAssignedPrivileges(collection, restriction, userId, writer);
                          writeDiscussionTagUserAssignedPrivileges(collection, restriction, userId, writer);
                          writeDiscussionThreadUserAssignedPrivileges(collection, restriction, userId, writer);

                          writer.endObject();

                          readEvents().onGetForumWideAssignedPrivilegesForUser(createObserverContext(currentUser),
                                                                               *userPtr);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilege(
        DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto oldValue = collection.DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

                           if ( ! (status = forumWideAuthorization_->updateDiscussionThreadMessagePrivilege(
                                   *currentUser, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadMessageRequiredPrivilegeForumWide(
                                   createObserverContext(*currentUser), privilege, value);

                           status = changeDiscussionThreadMessageRequiredPrivilege(collection, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageRequiredPrivilege(
        EntityCollection& collection, DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    collection.setDiscussionThreadMessagePrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadRequiredPrivilege(
        DiscussionThreadPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto oldValue = collection.DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);

                           if ( ! (status = forumWideAuthorization_->updateDiscussionThreadPrivilege(
                                   *currentUser, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionThreadRequiredPrivilegeForumWide(
                                   createObserverContext(*currentUser), privilege, value);

                           status = changeDiscussionThreadRequiredPrivilege(collection, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadRequiredPrivilege(
        EntityCollection& collection, DiscussionThreadPrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    collection.setDiscussionThreadPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionTagRequiredPrivilege(
        DiscussionTagPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto oldValue = collection.DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege);

                           if ( ! (status = forumWideAuthorization_->updateDiscussionTagPrivilege(
                                   *currentUser, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionTagRequiredPrivilegeForumWide(
                                   createObserverContext(*currentUser), privilege, value);

                           status = changeDiscussionTagRequiredPrivilege(collection, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionTagRequiredPrivilege(
        EntityCollection& collection, DiscussionTagPrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    collection.setDiscussionTagPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionCategoryRequiredPrivilege(
        DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto oldValue = collection.DiscussionCategoryPrivilegeStore::getDiscussionCategoryPrivilege(privilege);

                           if ( ! (status = forumWideAuthorization_->updateDiscussionCategoryPrivilege(
                                   *currentUser, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeDiscussionCategoryRequiredPrivilegeForumWide(
                                   createObserverContext(*currentUser), privilege, value);

                           status = changeDiscussionCategoryRequiredPrivilege(collection, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionCategoryRequiredPrivilege(
        EntityCollection& collection, DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    collection.setDiscussionCategoryPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeForumWideRequiredPrivilege(
        ForumWidePrivilege privilege, PrivilegeValueIntType value, OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto oldValue = collection.ForumWidePrivilegeStore::getForumWidePrivilege(privilege);

                           if ( ! (status = forumWideAuthorization_->updateForumWidePrivilege(
                                   *currentUser, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().onChangeForumWideRequiredPrivilege(
                                   createObserverContext(*currentUser), privilege, value);

                           status = changeForumWideRequiredPrivilege(collection, privilege, value);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeForumWideRequiredPrivilege(
        EntityCollection& collection, ForumWidePrivilege privilege, PrivilegeValueIntType value)
{
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeValue(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    collection.setForumWidePrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeForumWideDefaultPrivilegeLevel(
        ForumWideDefaultPrivilegeDuration privilege, PrivilegeValueIntType value, PrivilegeDurationIntType duration,
        OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeDuration(value))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           if ( ! (status = forumWideAuthorization_->updateForumWideDefaultPrivilegeLevel(
                                   *currentUser, privilege, value, duration)))
                           {
                               return;
                           }

                           writeEvents().onChangeForumWideDefaultPrivilegeLevel(
                                   createObserverContext(*currentUser), privilege, value, duration);

                           status = changeForumWideDefaultPrivilegeLevel(collection, privilege, value, duration);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::changeForumWideDefaultPrivilegeLevel(
        EntityCollection& collection, ForumWideDefaultPrivilegeDuration privilege,
        PrivilegeValueIntType value, PrivilegeDurationIntType duration)
{
    if ( ! isValidPrivilege(privilege) || ! isValidPrivilegeDuration(value))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    PrivilegeDefaultLevel level{};
    level.value = value;
    level.duration = duration;

    collection.setForumWideDefaultPrivilegeLevel(privilege, level);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignForumWidePrivilege(
        IdTypeRef userId, PrivilegeValueIntType value, PrivilegeDurationIntType duration, OutStream& output)
{
    StatusWriter status(output);
    if ( ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto targetUser = anonymousUser();
                           if (userId != anonymousUserId())
                           {
                               auto& userIndexById = collection.users().byId();
                               auto userIt = userIndexById.find(userId);
                               if (userIt == userIndexById.end())
                               {
                                   status = StatusCode::NOT_FOUND;
                                   return;
                               }
                               targetUser = *userIt;
                           }

                           if ( ! (status = forumWideAuthorization_->assignForumWidePrivilege(
                                   *currentUser, *targetUser, value)))
                           {
                               return;
                           }

                           writeEvents().onAssignForumWidePrivilege(
                                   createObserverContext(*currentUser), *targetUser, value, duration);

                           status = assignForumWidePrivilege(collection, userId, value, duration);
                       });
    return status;
}

StatusCode MemoryRepositoryAuthorization::assignForumWidePrivilege(
        EntityCollection& collection, IdTypeRef userId, PrivilegeValueIntType value,
        PrivilegeDurationIntType duration)
{
    if ( ! isValidPrivilegeValue(value) || ! isValidPrivilegeDuration(duration))
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    if (userId != anonymousUserId())
    {
        auto& userIndexById = collection.users().byId();
        auto userIt = userIndexById.find(userId);
        if (userIt == userIndexById.end())
        {
            FORUM_LOG_ERROR << "Could not find user: " << static_cast<std::string>(userId);
            return StatusCode::NOT_FOUND;
        }
    }

    const auto now = Context::getCurrentTime();
    const auto expiresAt = calculatePrivilegeExpires(now, duration);
    collection.grantedPrivileges().grantForumWidePrivilege(userId, {}, value, now, expiresAt);

    return StatusCode::OK;
}
