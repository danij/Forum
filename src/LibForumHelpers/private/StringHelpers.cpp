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

#include "StringHelpers.h"

#include <cstring>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>

#include <unicode/ucol.h>
#include <unicode/ustring.h>

using namespace Forum::Helpers;

struct LocaleCache final : private boost::noncopyable
{
public:
    static LocaleCache& instance()
    {
        std::call_once(initFlag_, []()
        {
            instance_ = std::unique_ptr<LocaleCache>(new LocaleCache);
        });
        return *instance_;
    }

    static void reset()
    {
        instance_.reset();
    }

    const UCollator* collator() const
    {
        return collator_.get();
    }

private:
    LocaleCache()
    {
        UErrorCode errorCode{};
        collator_.reset(ucol_open("en_US", &errorCode));

        if (U_FAILURE(errorCode))
        {
            throw std::runtime_error("Unable to load collation!");
        }

        ucol_setStrength(collator_.get(), UCollationStrength::UCOL_PRIMARY);
    }

    static std::once_flag initFlag_;
    static std::unique_ptr<LocaleCache> instance_;

    std::unique_ptr<UCollator, decltype(&ucol_close)> collator_ {nullptr, ucol_close};
};

std::once_flag LocaleCache::initFlag_;
std::unique_ptr<LocaleCache> LocaleCache::instance_;

static constexpr size_t MaxSortKeyGenerationUCharBufferSize = 65536;
static constexpr size_t MaxSortKeyGenerationDestinationBufferSize = 10 * MaxSortKeyGenerationUCharBufferSize;

static thread_local std::unique_ptr<UChar[]> SortKeyGenerationUCharBuffer(new UChar[MaxSortKeyGenerationUCharBufferSize]);
static thread_local std::unique_ptr<uint8_t[]> SortKeyGenerationDestinationBuffer(new uint8_t[MaxSortKeyGenerationDestinationBufferSize]);

static thread_local size_t CurrentSortKeyLength;

size_t Forum::Helpers::calculateSortKey(StringView view)
{
    if (view.empty())
    {
        SortKeyGenerationDestinationBuffer.get()[0] = 0;
        return CurrentSortKeyLength = 1;
    }

    if (view.size() > (MaxSortKeyGenerationUCharBufferSize / 8))
    {
        throw std::runtime_error("String for which a sort key is to be generated is too big");
    }

    const auto stringLength = view.size();

    int32_t u16Written{};
    UErrorCode errorCode{};

    auto ucharBuffer = SortKeyGenerationUCharBuffer.get();
    std::unique_ptr<UChar[]> ucharBufferIfThreadLocalOneIsNotYetInitialized;

    if ( ! ucharBuffer)
    {
        ucharBufferIfThreadLocalOneIsNotYetInitialized.reset(new UChar[MaxSortKeyGenerationUCharBufferSize]);
        ucharBuffer = ucharBufferIfThreadLocalOneIsNotYetInitialized.get();
    }

    auto u16Chars = u_strFromUTF8Lenient(ucharBuffer, MaxSortKeyGenerationUCharBufferSize,
                                         &u16Written, view.data(), static_cast<int32_t>(view.size()), &errorCode);
    if (U_FAILURE(errorCode))
    {
        //use string as sort key
        CurrentSortKeyLength = stringLength + 1;
        memmove(SortKeyGenerationDestinationBuffer.get(), view.data(), view.size());
        SortKeyGenerationDestinationBuffer[CurrentSortKeyLength] = 0;

        return CurrentSortKeyLength;
    }

    auto sortKeyBuffer = SortKeyGenerationDestinationBuffer.get();
    std::unique_ptr<uint8_t[]> sortKeyBufferIfThreadLocalOneIsNotYetInitialized;

    if ( ! sortKeyBuffer)
    {
        sortKeyBufferIfThreadLocalOneIsNotYetInitialized.reset(new uint8_t[MaxSortKeyGenerationDestinationBufferSize]);
        sortKeyBuffer = sortKeyBufferIfThreadLocalOneIsNotYetInitialized.get();
    }

    CurrentSortKeyLength = ucol_getSortKey(LocaleCache::instance().collator(), u16Chars, u16Written,
                                           sortKeyBuffer, MaxSortKeyGenerationDestinationBufferSize);

    return CurrentSortKeyLength;
}

char* Forum::Helpers::getCurrentSortKey()
{
    return reinterpret_cast<char*>(SortKeyGenerationDestinationBuffer.get());
}

size_t Forum::Helpers::getCurrentSortKeyLength()
{
    return CurrentSortKeyLength;
}

void Forum::Helpers::cleanupStringHelpers()
{
    LocaleCache::reset();
}
