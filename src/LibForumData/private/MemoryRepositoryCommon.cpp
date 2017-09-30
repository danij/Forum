#include "MemoryRepositoryCommon.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

#include <algorithm>
#include <tuple>
#include <type_traits>

#include <unicode/uchar.h>
#include <unicode/ustring.h>

#include <boost/optional.hpp>

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

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

static boost::optional<std::tuple<uint32_t, uint32_t>> getPNGSize(StringView content)
{
    return{};//TODO: update
}

StatusCode MemoryRepositoryBase::validateImage(StringView content, uint_fast32_t maxBinarySize, uint_fast32_t maxWidth,
                                               uint_fast32_t maxHeight)
{
    if (content.size() < 1)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }
    if (content.size() > maxBinarySize)
    {
        return StatusCode::VALUE_TOO_LONG;
    }

    auto pngSize = getPNGSize(content);
    if ( ! pngSize)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    if ((std::get<0>(*pngSize) < 1) || (std::get<1>(*pngSize) < 1))
    {
        return StatusCode::VALUE_TOO_SHORT;
    }
    if ((std::get<0>(*pngSize) > maxWidth) || (std::get<1>(*pngSize) > maxHeight))
    {
        return StatusCode::VALUE_TOO_LONG;
    }

    return StatusCode::OK;
}
