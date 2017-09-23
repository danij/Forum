#include "MemoryRepositoryAuthorization.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

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
        auto value = (store.*fn)(privilege);
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
    writer.newPropertyWithSafeName("discussion_thread_message_privileges");
    writePrivilegeValues(store, DiscussionThreadMessagePrivilege::COUNT, writer, DiscussionThreadMessagePrivilegeStrings,
                         &DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege);
}

void MemoryRepositoryAuthorization::writeDiscussionThreadRequiredPrivileges(const DiscussionThreadPrivilegeStore& store,
                                                                            JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_privileges");
    writePrivilegeValues(store, DiscussionThreadPrivilege::COUNT, writer, DiscussionThreadPrivilegeStrings,
                         &DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege);
}

void MemoryRepositoryAuthorization::writeDiscussionTagRequiredPrivileges(const DiscussionTagPrivilegeStore& store,
                                                                         JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_tag_privileges");
    writePrivilegeValues(store, DiscussionTagPrivilege::COUNT, writer, DiscussionTagPrivilegeStrings,
                         &DiscussionTagPrivilegeStore::getDiscussionTagPrivilege);
}

void MemoryRepositoryAuthorization::writeDiscussionCategoryRequiredPrivileges(const DiscussionCategoryPrivilegeStore& store,
                                                                              JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_category_privileges");
    writePrivilegeValues(store, DiscussionCategoryPrivilege::COUNT, writer, DiscussionCategoryPrivilegeStrings,
                         &DiscussionCategoryPrivilegeStore::getDiscussionCategoryPrivilege);
}

void MemoryRepositoryAuthorization::writeForumWideRequiredPrivileges(const ForumWidePrivilegeStore& store,
                                                                     JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forum_wide_privileges");
    writePrivilegeValues(store, ForumWidePrivilege::COUNT, writer, ForumWidePrivilegeStrings,
                         &ForumWidePrivilegeStore::getForumWidePrivilege);
}

void MemoryRepositoryAuthorization::writeDiscussionThreadMessageDefaultPrivilegeDurations(const DiscussionThreadPrivilegeStore& store,
                                                                                          JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_message_default_durations");
    writePrivilegeValues(store, DiscussionThreadMessageDefaultPrivilegeDuration::COUNT, writer,
                         DiscussionThreadMessageDefaultPrivilegeDurationStrings,
                         &DiscussionThreadPrivilegeStore::getDiscussionThreadMessageDefaultPrivilegeDuration);
}

void MemoryRepositoryAuthorization::writeForumWideDefaultPrivilegeDurations(const ForumWidePrivilegeStore& store,
                                                                            JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forum_wide_default_durations");
    writePrivilegeValues(store, ForumWideDefaultPrivilegeDuration::COUNT, writer,
                         ForumWideDefaultPrivilegeDurationStrings,
                         &ForumWidePrivilegeStore::getForumWideDefaultPrivilegeDuration);
}

struct AssignedPrivilegeWriter final
{
    AssignedPrivilegeWriter(JsonWriter& writer, const EntityCollection& collection, const StringView* strings)
            : writer_(writer), collection_(collection), strings_(strings)
    {
    }

    AssignedPrivilegeWriter(const AssignedPrivilegeWriter&) = default;
    AssignedPrivilegeWriter(AssignedPrivilegeWriter&&) = default;

    AssignedPrivilegeWriter& operator=(const AssignedPrivilegeWriter&) = default;
    AssignedPrivilegeWriter& operator=(AssignedPrivilegeWriter&&) = default;

    void operator()(IdTypeRef userId, EnumIntType privilege, PrivilegeValueIntType privilegeValue, Timestamp expiresAt)
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
            auto it = index.find(userId);
            if (it != index.end())
            {
                writer_.newPropertyWithSafeName("name") << (**it).name();
            }
        }

        writer_.newPropertyWithSafeName("privilege") << strings_[privilege];
        writer_.newPropertyWithSafeName("value") << strings_[privilegeValue];
        writer_.newPropertyWithSafeName("expires") << strings_[expiresAt];
        writer_.endObject();
    }
private:
    JsonWriter& writer_;
    const EntityCollection& collection_;
    const StringView* strings_;
};

struct UserAssignedPrivilegeWriter final
{
    typedef void(*WriteNameFunction)(const EntityCollection& collection, IdTypeRef entityId, JsonWriter& writer);

    UserAssignedPrivilegeWriter(JsonWriter& writer, const EntityCollection& collection, const StringView* strings,
                                WriteNameFunction writeName)
            : writer_(writer), collection_(collection), strings_(strings), writeName_(writeName)
    {
    }

    UserAssignedPrivilegeWriter(const UserAssignedPrivilegeWriter&) = default;
    UserAssignedPrivilegeWriter(UserAssignedPrivilegeWriter&&) = default;

    UserAssignedPrivilegeWriter& operator=(const UserAssignedPrivilegeWriter&) = default;
    UserAssignedPrivilegeWriter& operator=(UserAssignedPrivilegeWriter&&) = default;

    void operator()(IdTypeRef entityId, EnumIntType privilege, PrivilegeValueIntType privilegeValue, Timestamp expiresAt)
    {
        writer_.startObject();
        writer_.newPropertyWithSafeName("id") << entityId;

        writeName_(collection_, entityId, writer_);

        writer_.newPropertyWithSafeName("privilege") << strings_[privilege];
        writer_.newPropertyWithSafeName("value") << strings_[privilegeValue];
        writer_.newPropertyWithSafeName("expires") << strings_[expiresAt];
        writer_.endObject();
    }
private:
    JsonWriter& writer_;
    const EntityCollection& collection_;
    const StringView* strings_;
    WriteNameFunction writeName_;
};

static void writeDiscussionThreadName(const EntityCollection& collection, IdTypeRef entityId, JsonWriter& writer)
{
    const auto& index = collection.threads().byId();
    auto it = index.find(entityId);
    if (it != index.end())
    {
        writer.newPropertyWithSafeName("name") << (**it).name();
    }
}

static void writeDiscussionTagName(const EntityCollection& collection, IdTypeRef entityId, JsonWriter& writer)
{
    const auto& index = collection.tags().byId();
    auto it = index.find(entityId);
    if (it != index.end())
    {
        writer.newPropertyWithSafeName("name") << (**it).name();
    }
}

static void writeDiscussionCategoryName(const EntityCollection& collection, IdTypeRef entityId, JsonWriter& writer)
{
    const auto& index = collection.categories().byId();
    auto it = index.find(entityId);
    if (it != index.end())
    {
        writer.newPropertyWithSafeName("name") << (**it).name();
    }
}

static void writeForumWideName(const EntityCollection& collection, IdTypeRef entityId, JsonWriter& writer)
{
    //do nothing
}

void MemoryRepositoryAuthorization::writeDiscussionThreadMessageAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                                   JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_message_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionThreadMessagePrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionThreadMessagePrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionThreadAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                            JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionThreadPrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionThreadPrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionTagAssignedPrivileges(const EntityCollection& collection,
                                                                         IdTypeRef id, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_tag_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionTagPrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionTagPrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionCategoryAssignedPrivileges(const EntityCollection& collection,
                                                                              IdTypeRef id, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_category_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionCategoryPrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionCategoryPrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeForumWideAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                     JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forum_wide_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateForumWidePrivileges(id,
            AssignedPrivilegeWriter(writer, collection, ForumWidePrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionThreadUserAssignedPrivileges(const EntityCollection& collection,
                                                                                IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionThreadPrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, DiscussionThreadPrivilegeStrings, writeDiscussionThreadName));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionTagUserAssignedPrivileges(const EntityCollection& collection,
                                                                             IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_tag_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionTagPrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, DiscussionTagPrivilegeStrings, writeDiscussionTagName));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeDiscussionCategoryUserAssignedPrivileges(const EntityCollection& collection,
                                                                                  IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_category_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateDiscussionCategoryPrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, DiscussionCategoryPrivilegeStrings, writeDiscussionCategoryName));

    writer.endArray();
}

void MemoryRepositoryAuthorization::writeForumWideUserAssignedPrivileges(const EntityCollection& collection,
                                                                         IdTypeRef userId, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forum_wide_privileges");
    writer.startArray();

    collection.grantedPrivileges().enumerateForumWidePrivilegesAssignedToUser(userId,
            UserAssignedPrivilegeWriter(writer, collection, ForumWidePrivilegeStrings, writeForumWideName));

    writer.endArray();
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

                          if ( ! (status = threadMessageAuthorization_->getDiscussionThreadMessageById(currentUser, message)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadMessageRequiredPrivileges(message, writer);

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

                          if ( ! (status = threadMessageAuthorization_->getDiscussionThreadMessageById(currentUser, message)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadMessageAssignedPrivileges(collection, message.id(), writer);

                          writer.endObject();

                          readEvents().onGetAssignedPrivilegesFromThreadMessage(createObserverContext(currentUser),
                                                                                message);
                      });
    return status;
}

template<typename EnumType>
bool isValidPrivilege(EnumType value)
{
    auto intValue = static_cast<EnumIntType>(value);
    return intValue >= 0 && intValue < static_cast<EnumIntType>(EnumType::COUNT);
}

static bool isValidPrivilegeValue(PrivilegeValueIntType value)
{
    return value >= MinPrivilegeValue && value <= MaxPrivilegeValue;
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

                           writeEvents().changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
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
    auto it = indexById.find(messageId);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionThreadMessagePtr message = *it;
    message->setDiscussionThreadMessagePrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilegeToDiscussionThreadMessage(
        IdTypeRef messageId, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilegeToDiscussionThreadMessage(
        EntityCollection& collection,IdTypeRef messageId, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
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

                          const auto& index = collection.threads().byId();
                          auto it = index.find(threadId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& thread = **it;

                          if ( ! (status = threadAuthorization_->getDiscussionThreadById(currentUser, thread)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadRequiredPrivileges(thread, writer);
                          writeDiscussionThreadMessageRequiredPrivileges(thread, writer);

                          writer.endObject();

                          readEvents().onGetRequiredPrivilegesFromThread(createObserverContext(currentUser), thread);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getDefaultPrivilegeDurationsForThread(IdTypeRef threadId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.threads().byId();
                          auto it = index.find(threadId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& thread = **it;

                          if ( ! (status = threadAuthorization_->getDiscussionThreadById(currentUser, thread)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadMessageDefaultPrivilegeDurations(thread, writer);

                          writer.endObject();

                          readEvents().onGetDefaultPrivilegeDurationsFromThread(createObserverContext(currentUser), thread);
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

                          const auto& index = collection.threads().byId();
                          auto it = index.find(threadId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& thread = **it;

                          if ( ! (status = threadAuthorization_->getDiscussionThreadById(currentUser, thread)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadAssignedPrivileges(collection, thread.id(), writer);
                          writeDiscussionThreadMessageAssignedPrivileges(collection, thread.id(), writer);

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

                           auto& indexById = collection.threads().byId();
                           auto it = indexById.find(threadId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionThread& thread = **it;
                           auto oldValue = thread.DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

                           if ( ! (status = threadAuthorization_->updateDiscussionThreadMessagePrivilege(
                                   *currentUser, thread, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().changeDiscussionThreadMessageRequiredPrivilegeForThread(
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

    auto& indexById = collection.threads().byId();
    auto it = indexById.find(threadId);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionThreadPtr thread = *it;
    thread->setDiscussionThreadMessagePrivilege(privilege, value);

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

                           auto& indexById = collection.threads().byId();
                           auto it = indexById.find(threadId);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           const DiscussionThread& thread = **it;
                           auto oldValue = thread.DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);

                           if ( ! (status = threadAuthorization_->updateDiscussionThreadPrivilege(
                                   *currentUser, thread, privilege, oldValue, value)))
                           {
                               return;
                           }

                           writeEvents().changeDiscussionThreadRequiredPrivilegeForThread(
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

    auto& indexById = collection.threads().byId();
    auto it = indexById.find(threadId);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionThreadPtr thread = *it;
    thread->setDiscussionThreadPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(
        IdTypeRef threadId, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(
        EntityCollection& collection, IdTypeRef threadId, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value)
{
    return {};
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilegeForThread(
        IdTypeRef threadId, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilegeForThread(
        EntityCollection& collection, IdTypeRef threadId, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilegeForThread(
        IdTypeRef threadId, IdTypeRef userId, DiscussionThreadPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilegeForThread(
        EntityCollection& collection, IdTypeRef threadId, IdTypeRef userId, DiscussionThreadPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
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

                          if ( ! (status = tagAuthorization_->getDiscussionTagById(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionTagRequiredPrivileges(tag, writer);
                          writeDiscussionThreadRequiredPrivileges(tag, writer);
                          writeDiscussionThreadMessageRequiredPrivileges(tag, writer);

                          writer.endObject();

                          readEvents().onGetRequiredPrivilegesFromTag(createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getDefaultPrivilegeDurationsForTag(IdTypeRef tagId, OutStream& output) const
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

                          if ( ! (status = tagAuthorization_->getDiscussionTagById(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionThreadMessageDefaultPrivilegeDurations(tag, writer);

                          writer.endObject();

                          readEvents().onGetDefaultPrivilegeDurationsFromTag(createObserverContext(currentUser), tag);
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

                          if ( ! (status = tagAuthorization_->getDiscussionTagById(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionTagAssignedPrivileges(collection, tag.id(), writer);
                          writeDiscussionThreadAssignedPrivileges(collection, tag.id(), writer);
                          writeDiscussionThreadMessageAssignedPrivileges(collection, tag.id(), writer);

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

                           writeEvents().changeDiscussionThreadMessageRequiredPrivilegeForTag(
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

                           writeEvents().changeDiscussionThreadRequiredPrivilegeForTag(
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

                           writeEvents().changeDiscussionTagRequiredPrivilegeForTag(
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
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tag = *it;
    tag->setDiscussionTagPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(
        IdTypeRef tagId, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(
        EntityCollection& collection, IdTypeRef tagId, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value)
{
    return {};
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilegeForTag(
        IdTypeRef tagId, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilegeForTag(
        EntityCollection& collection, IdTypeRef tagId, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilegeForTag(
        IdTypeRef tagId, IdTypeRef userId, DiscussionThreadPrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilegeForTag(
        EntityCollection& collection, IdTypeRef tagId, IdTypeRef userId, DiscussionThreadPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionTagPrivilegeForTag(
        IdTypeRef tagId, IdTypeRef userId, DiscussionTagPrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionTagPrivilegeForTag(
        EntityCollection& collection, IdTypeRef tagId, IdTypeRef userId, DiscussionTagPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
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

                          if ( ! (status = categoryAuthorization_->getDiscussionCategoryById(currentUser, category)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionCategoryRequiredPrivileges(category, writer);

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

                          if ( ! (status = categoryAuthorization_->getDiscussionCategoryById(currentUser, category)))
                          {
                              return;
                          }

                          status.disable();

                          JsonWriter writer(output);
                          writer.startObject();

                          writeDiscussionCategoryAssignedPrivileges(collection, category.id(), writer);

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

                           writeEvents().changeDiscussionCategoryRequiredPrivilegeForCategory(
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
        return StatusCode::NOT_FOUND;
    }

    DiscussionCategoryPtr category = *it;
    category->setDiscussionCategoryPrivilege(privilege, value);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionCategoryPrivilege(
        IdTypeRef categoryId, IdTypeRef userId, DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionCategoryPrivilege(
        EntityCollection& collection, IdTypeRef categoryId, IdTypeRef userId, DiscussionCategoryPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}

//
//forum wide
//


StatusCode MemoryRepositoryAuthorization::getCurrentUserPrivileges(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          status = StatusCode::OK;
                          status.disable();

                          auto& currentUser = performedBy.get(collection, *store_);

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(),
                                                               Context::getCurrentTime());

                          JsonWriter writer(output);
                          writer.startObject();

                          writePrivileges(writer, collection, ForumWidePrivilegesToSerialize, restriction);

                          writer.endObject();

                          readEvents().onGetCurrentUserPrivileges(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getRequiredPrivileges(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          status = StatusCode::OK;
                          status.disable();

                          auto& currentUser = performedBy.get(collection, *store_);

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideRequiredPrivileges(collection, writer);
                          writeDiscussionCategoryRequiredPrivileges(collection, writer);
                          writeDiscussionTagRequiredPrivileges(collection, writer);
                          writeDiscussionThreadRequiredPrivileges(collection, writer);
                          writeDiscussionThreadMessageRequiredPrivileges(collection, writer);

                          writer.endObject();

                          readEvents().onGetForumWideRequiredPrivileges(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getDefaultPrivilegeDurations(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          status = StatusCode::OK;
                          status.disable();

                          auto& currentUser = performedBy.get(collection, *store_);

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideDefaultPrivilegeDurations(collection, writer);
                          writeDiscussionThreadMessageDefaultPrivilegeDurations(collection, writer);

                          writer.endObject();

                          readEvents().onGetForumWideDefaultPrivilegeDurations(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivileges(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          status = StatusCode::OK;
                          status.disable();

                          auto& currentUser = performedBy.get(collection, *store_);

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideAssignedPrivileges(collection, {}, writer);
                          writeDiscussionCategoryAssignedPrivileges(collection, {}, writer);
                          writeDiscussionTagAssignedPrivileges(collection, {}, writer);
                          writeDiscussionThreadAssignedPrivileges(collection, {}, writer);
                          writeDiscussionThreadMessageAssignedPrivileges(collection, {}, writer);

                          writer.endObject();

                          readEvents().onGetForumWideAssignedPrivileges(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryAuthorization::getAssignedPrivileges(IdTypeRef userId, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          status = StatusCode::OK;
                          status.disable();

                          if (userId != anonymousUserId())
                          {
                              const auto& indexById = collection.users().byId();
                              auto it = indexById.find(userId);
                              if (it == indexById.end())
                              {
                                  status = StatusCode::NOT_FOUND;
                                  return;
                              }
                          }

                          auto& currentUser = performedBy.get(collection, *store_);

                          JsonWriter writer(output);
                          writer.startObject();

                          writeForumWideUserAssignedPrivileges(collection, userId, writer);
                          writeDiscussionCategoryUserAssignedPrivileges(collection, userId, writer);
                          writeDiscussionTagUserAssignedPrivileges(collection, userId, writer);
                          writeDiscussionThreadUserAssignedPrivileges(collection, userId, writer);

                          writer.endObject();

                          readEvents().onGetForumWideAssignedPrivileges(createObserverContext(currentUser));
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

                           writeEvents().changeDiscussionThreadMessageRequiredPrivilegeForumWide(
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

                           writeEvents().changeDiscussionThreadRequiredPrivilegeForumWide(
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

                           writeEvents().changeDiscussionTagRequiredPrivilegeForumWide(
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

                           writeEvents().changeDiscussionCategoryRequiredPrivilegeForumWide(
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

                           writeEvents().changeForumWideRequiredPrivilege(
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

StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageDefaultPrivilegeDuration(
        DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::changeDiscussionThreadMessageDefaultPrivilegeDuration(
        EntityCollection& collection, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::changeForumWideMessageDefaultPrivilegeDuration(
        ForumWideDefaultPrivilegeDuration privilege, PrivilegeDefaultDurationIntType value, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::changeForumWideMessageDefaultPrivilegeDuration(
        EntityCollection& collection, ForumWideDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType value)
{
    return {};
}

StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilege(
        IdTypeRef userId, DiscussionThreadMessagePrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadMessagePrivilege(
        EntityCollection& collection, IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilege(
        IdTypeRef userId, DiscussionThreadPrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionThreadPrivilege(
        EntityCollection& collection, IdTypeRef userId, DiscussionThreadPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionTagPrivilege(
        IdTypeRef userId, DiscussionTagPrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionTagPrivilege(
        EntityCollection& collection, IdTypeRef userId, DiscussionTagPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionCategoryPrivilege(
        IdTypeRef userId, DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignDiscussionCategoryPrivilege(
        EntityCollection& collection, IdTypeRef userId, DiscussionCategoryPrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignForumWidePrivilege(
        IdTypeRef userId, ForumWidePrivilege privilege, PrivilegeValueIntType value,
        PrivilegeDefaultDurationIntType duration, OutStream& output)
{
    return {};
}
StatusCode MemoryRepositoryAuthorization::assignForumWidePrivilege(
        EntityCollection& collection, IdTypeRef userId, ForumWidePrivilege privilege,
        PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
{
    return {};
}
