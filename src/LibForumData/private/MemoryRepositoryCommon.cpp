#include "MemoryRepositoryCommon.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

#include <algorithm>
#include <type_traits>

#include <unicode/uchar.h>
#include <unicode/ustring.h>

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

using namespace Json;

/**
 * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
 * The update is performed on the spot if a write lock is held or
 * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
 * Do not keep references to it outside of MemoryRepository methods
 */
PerformedByWithLastSeenUpdateGuard::~PerformedByWithLastSeenUpdateGuard()
{
    if (lastSeenUpdate_) lastSeenUpdate_();
}

PerformedByType PerformedByWithLastSeenUpdateGuard::get(const EntityCollection& collection, const MemoryStore& store)
{
    const auto& index = collection.users().byId();
    auto it = index.find(Context::getCurrentUserId());
    if (it == index.end())
    {
        return *anonymousUser();
    }

    const User& result = **it;

    auto now = Context::getCurrentTime();

    if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        auto& userId = result.id();
        const auto& mutableCollection = store.collection;
        lastSeenUpdate_ = [&mutableCollection, now, &userId]()
                          {
                              mutableCollection.write([&](EntityCollection& collectionToModify)
                              {
                                  auto& indexToModify = collectionToModify.users().byId();
                                  auto itToModify = indexToModify.find(userId);
                                  if (itToModify != indexToModify.end())
                                  {
                                      UserPtr userToModify = *itToModify;
                                      userToModify->updateLastSeen(now);
                                  }
                              });
                          };
    }
    return result;
}

UserPtr PerformedByWithLastSeenUpdateGuard::getAndUpdate(EntityCollection& collection)
{
    lastSeenUpdate_ = nullptr;

    auto result = getCurrentUser(collection);
    if (result == anonymousUser())
    {
        return result;
    }

    auto now = Context::getCurrentTime();

    if ((result->lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        result->updateLastSeen(now);
    }
    return result;
}

UserPtr Repository::getCurrentUser(EntityCollection& collection)
{
    const auto& index = collection.users().byId();
    auto it = index.find(Context::getCurrentUserId());
    if (it == index.end())
    {
        return anonymousUser();
    }
    return *it;
}

StatusCode MemoryRepositoryBase::validateString(StringView string,
                                                EmptyStringValidation emptyValidation,
                                                boost::optional<int_fast32_t> minimumLength,
                                                boost::optional<int_fast32_t> maximumLength)
{
    if ((INVALID_PARAMETERS_FOR_EMPTY_STRING == emptyValidation) && string.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto nrCharacters = countUTF8Characters(string);
    if (maximumLength && (nrCharacters > *maximumLength))
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (minimumLength && (nrCharacters < *minimumLength))
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    return StatusCode::OK;
}

bool MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace(StringView& input)
{
    if (input.size() < 1)
    {
        return true;
    }

    char firstLastUtf8[2 * 4];
    UChar temp[2 * 4];
    UChar32 u32Buffer[2];

    int32_t written;
    UErrorCode errorCode{};

    auto firstCharView = getFirstCharacterInUTF8Array(input);
    auto lastCharView = getLastCharacterInUTF8Array(input);

    auto nrOfFirstCharBytes = std::min(static_cast<StringView::size_type>(4), firstCharView.size());
    auto nrOfLastCharBytes = std::min(static_cast<StringView::size_type>(4), lastCharView.size());

    std::copy(firstCharView.data(), firstCharView.data() + nrOfFirstCharBytes, firstLastUtf8);
    std::copy(lastCharView.data(), lastCharView.data() + nrOfLastCharBytes, firstLastUtf8 + nrOfFirstCharBytes);

    auto u16Chars = u_strFromUTF8(temp, std::extent<decltype(temp)>::value, &written,
                                  firstLastUtf8, nrOfFirstCharBytes + nrOfLastCharBytes, &errorCode);
    if (U_FAILURE(errorCode)) return false;

    errorCode = {};
    auto u32Chars = u_strToUTF32(u32Buffer, 2, &written, u16Chars, written, &errorCode);
    if (U_FAILURE(errorCode)) return false;

    return (u_isUWhiteSpace(u32Chars[0]) == FALSE) && (u_isUWhiteSpace(u32Chars[1]) == FALSE);
}

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

void MemoryRepositoryBase::writeDiscussionThreadMessageRequiredPrivileges(
        const DiscussionThreadMessagePrivilegeStore& store, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_message_privileges");
    writePrivilegeValues(store, DiscussionThreadMessagePrivilege::COUNT, writer, DiscussionThreadMessagePrivilegeStrings, 
                         &DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege);
}

void MemoryRepositoryBase::writeDiscussionThreadRequiredPrivileges(const DiscussionThreadPrivilegeStore& store, 
                                                                   JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_privileges");
    writePrivilegeValues(store, DiscussionThreadPrivilege::COUNT, writer, DiscussionThreadPrivilegeStrings,
                         &DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege);
}

void MemoryRepositoryBase::writeDiscussionTagRequiredPrivileges(const DiscussionTagPrivilegeStore& store, 
                                                                JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_tag_privileges");
    writePrivilegeValues(store, DiscussionTagPrivilege::COUNT, writer, DiscussionTagPrivilegeStrings,
                         &DiscussionTagPrivilegeStore::getDiscussionTagPrivilege);
}

void MemoryRepositoryBase::writeDiscussionCategoryRequiredPrivileges(const DiscussionCategoryPrivilegeStore& store,
                                                                     JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_category_privileges");
    writePrivilegeValues(store, DiscussionCategoryPrivilege::COUNT, writer, DiscussionCategoryPrivilegeStrings,
                         &DiscussionCategoryPrivilegeStore::getDiscussionCategoryPrivilege);
}

void MemoryRepositoryBase::writeForumWideRequiredPrivileges(const ForumWidePrivilegeStore& store, JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forum_wide_privileges");
    writePrivilegeValues(store, ForumWidePrivilege::COUNT, writer, ForumWidePrivilegeStrings,
                         &ForumWidePrivilegeStore::getForumWidePrivilege);
}

void MemoryRepositoryBase::writeDiscussionThreadMessageDefaultPrivilegeDurations(const DiscussionThreadPrivilegeStore& store,
                                                                                 JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_message_default_durations");
    writePrivilegeValues(store, DiscussionThreadMessageDefaultPrivilegeDuration::COUNT, writer,
                         DiscussionThreadMessageDefaultPrivilegeDurationStrings,
                         &DiscussionThreadPrivilegeStore::getDiscussionThreadMessageDefaultPrivilegeDuration);
}

void MemoryRepositoryBase::writeForumWideDefaultPrivilegeDurations(const ForumWidePrivilegeStore& store, 
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

void MemoryRepositoryBase::writeDiscussionThreadMessageAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                          const IDiscussionThreadMessageEnumeratePrivileges& store, 
                                                                          JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_message_privileges");
    writer.startArray();

    store.enumerateDiscussionThreadMessagePrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionThreadMessagePrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryBase::writeDiscussionThreadAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                   const IDiscussionThreadEnumeratePrivileges& store, 
                                                                   JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_thread_privileges");
    writer.startArray();

    store.enumerateDiscussionThreadPrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionThreadPrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryBase::writeDiscussionTagAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                const IDiscussionTagEnumeratePrivileges& store, 
                                                                JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_tag_privileges");
    writer.startArray();

    store.enumerateDiscussionTagPrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionTagPrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryBase::writeDiscussionCategoryAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                                     const IDiscussionCategoryEnumeratePrivileges& store, 
                                                                     JsonWriter& writer)
{
    writer.newPropertyWithSafeName("discussion_category_privileges");
    writer.startArray();

    store.enumerateDiscussionCategoryPrivileges(id,
            AssignedPrivilegeWriter(writer, collection, DiscussionCategoryPrivilegeStrings));

    writer.endArray();
}

void MemoryRepositoryBase::writeForumWideAssignedPrivileges(const EntityCollection& collection, IdTypeRef id,
                                                            const IForumWideEnumeratePrivileges& store, 
                                                            JsonWriter& writer)
{
    writer.newPropertyWithSafeName("forum_wide_privileges");
    writer.startArray();

    store.enumerateForumWidePrivileges(id,
            AssignedPrivilegeWriter(writer, collection, ForumWidePrivilegeStrings));

    writer.endArray();
}
