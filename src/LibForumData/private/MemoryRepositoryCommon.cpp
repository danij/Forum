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

#include "MemoryRepositoryCommon.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "TypeHelpers.h"

#include <algorithm>
#include <tuple>

#include <unicode/uchar.h>
#include <unicode/ustring.h>

#include <boost/endian/conversion.hpp>
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
    const auto it = index.find(Context::getCurrentUserId());
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

    const auto now = Context::getCurrentTime();

    if ((result->lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        result->updateLastSeen(now);
    }
    return result;
}

UserPtr Repository::getCurrentUser(EntityCollection& collection)
{
    const auto& index = collection.users().byId();
    const auto it = index.find(Context::getCurrentUserId());
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

    const auto nrCharacters = countUTF8Characters(string);
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
    if (input.empty())
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

    const auto nrOfFirstCharBytes = std::min(static_cast<StringView::size_type>(4), firstCharView.size());
    const auto nrOfLastCharBytes = std::min(static_cast<StringView::size_type>(4), lastCharView.size());

    std::copy(firstCharView.data(), firstCharView.data() + nrOfFirstCharBytes, firstLastUtf8);
    std::copy(lastCharView.data(), lastCharView.data() + nrOfLastCharBytes, firstLastUtf8 + nrOfFirstCharBytes);

    const auto u16Chars = u_strFromUTF8(temp, static_cast<int32_t>(std::size(temp)), &written, firstLastUtf8, 
                                        static_cast<int32_t>(nrOfFirstCharBytes + nrOfLastCharBytes), &errorCode);
    if (U_FAILURE(errorCode)) return false;

    errorCode = {};
    const auto u32Chars = u_strToUTF32(u32Buffer, 2, &written, u16Chars, written, &errorCode);
    if (U_FAILURE(errorCode)) return false;

    return (u_isUWhiteSpace(u32Chars[0]) == FALSE) && (u_isUWhiteSpace(u32Chars[1]) == FALSE);
}

static boost::optional<std::tuple<uint32_t, uint32_t>> getPNGSize(StringView content)
{
    static const unsigned char PNGStart[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    static const unsigned char PNGIHDR[] = { 0x49, 0x48, 0x44, 0x52 };

    const auto requiredSize = std::size(PNGStart)
                              + 4  //size
                              + std::size(PNGIHDR)
                              + 4  //width
                              + 4; //height

    if (content.size() < requiredSize)
    {
        return{};
    }

    auto data = reinterpret_cast<const unsigned char*>(content.data());

    if ( ! std::equal(std::begin(PNGStart), std::end(PNGStart), data))
    {
        return{};
    }
    data += std::size(PNGStart);
    data += 4;
    if ( ! std::equal(std::begin(PNGIHDR), std::end(PNGIHDR), data))
    {
        return{};
    }
    data += std::size(PNGIHDR);

    uint32_t width, height;
    readValue(data, width);

    data += 4;
    readValue(data, height);

    boost::endian::big_to_native_inplace(width);
    boost::endian::big_to_native_inplace(height);

    return std::make_tuple(width, height);
}

StatusCode MemoryRepositoryBase::validateImage(StringView content, const uint_fast32_t maxBinarySize, 
                                               const uint_fast32_t maxWidth, uint_fast32_t maxHeight)
{
    if (content.empty())
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
