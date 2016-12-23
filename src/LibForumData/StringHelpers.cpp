#include "StringHelpers.h"

#include <boost/locale/collator.hpp>

thread_local const boost::locale::generator Forum::Helpers::localeGenerator;
thread_local const std::locale Forum::Helpers::en_US_UTF8Locale = Forum::Helpers::localeGenerator("en_US.UTF-8");
thread_local boost::locale::comparator<char> stringPrimaryComparator(Forum::Helpers::en_US_UTF8Locale,
                                                                     boost::locale::collator_base::level_type::primary);

bool Forum::Helpers::StringAccentAndCaseInsensitiveLess::operator()(const std::string& lhs,
                                                                    const std::string& rhs) const
{
    return stringPrimaryComparator(lhs, rhs);
}
