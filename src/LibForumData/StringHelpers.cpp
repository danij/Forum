#include "StringHelpers.h"

#include <boost/locale.hpp>
#include <boost/locale/collator.hpp>
#include <memory>
#include <mutex>

struct LocaleCache
{
    const boost::locale::generator localeGenerator;
    const std::locale en_US_UTF8Locale{ localeGenerator("en_US.UTF-8") };
    const boost::locale::comparator<char> stringPrimaryComparator{ en_US_UTF8Locale,
                                                                  boost::locale::collator_base::level_type::primary };
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

bool Forum::Helpers::StringAccentAndCaseInsensitiveLess::operator()(const std::string& lhs,
                                                                    const std::string& rhs) const
{
    return localeCache->stringPrimaryComparator(lhs, rhs);
}

void Forum::Helpers::cleanupStringHelpers()
{
    localeCache.reset();
}
