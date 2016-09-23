#include <boost/locale.hpp>
#include <boost/regex/icu.hpp>

#include "MemoryRepository.h"
#include "OutputHelpers.h"
#include "StringHelpers.h"
#include "Configuration.h"

using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

MemoryRepository::MemoryRepository() : collection_(std::make_shared<EntityCollection>())
{
}

void MemoryRepository::getUserCount(std::ostream& output) const
{
    std::size_t count;
    collection_.read([&](const EntityCollection& collection)
                     {
                         count = collection.usersById().size();
                     });
    writeSingleValueSafeName(output, "count", count);
}

const auto validUserNameRegex = boost::make_u32regex("^[[:alnum:]]+[ _-]*[[:alnum:]]+$");

StatusCode MemoryRepository::addNewUser(const std::string& name, std::ostream& output)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    if ( ! boost::u32regex_match(name, validUserNameRegex, boost::match_flag_type::format_all))
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    auto config = Configuration::getGlobalConfig();

    if (countUTF8Characters(name) > config->user.maxNameLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }

    return StatusCode::OK;
}
