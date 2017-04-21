#include "StringHelpers.h"

#include <memory>
#include <mutex>
#include <stdexcept>

#include <boost/noncopyable.hpp>

#include <unicode/ucol.h>

struct LocaleCache final : private boost::noncopyable
{
    std::unique_ptr<UCollator, decltype(&ucol_close)> collator {nullptr, ucol_close};

    LocaleCache()
    {
        UErrorCode errorCode{};
        collator.reset(ucol_open("en_US", &errorCode));
        
        if (U_FAILURE(errorCode))
        {
            throw std::runtime_error("Unable to load collation!");
        }

        ucol_setStrength(collator.get(), UCollationStrength::UCOL_PRIMARY);
    }
};

static std::unique_ptr<LocaleCache> localeCache;
static std::once_flag localeCacheInitFlag;

Forum::Helpers::StringAccentAndCaseInsensitiveLess::StringAccentAndCaseInsensitiveLess()
{
    std::call_once(localeCacheInitFlag, []()
    {
        localeCache = std::make_unique<LocaleCache>();
    });
}

bool Forum::Helpers::StringAccentAndCaseInsensitiveLess::operator()(StringView lhs, StringView rhs) const
{
    UErrorCode errorCode{};
    auto result = ucol_strcollUTF8(localeCache->collator.get(), lhs.data(), lhs.size(), rhs.data(), rhs.size(), &errorCode);
    return UCOL_LESS == result;
}

void Forum::Helpers::cleanupStringHelpers()
{
    localeCache.reset();
}
