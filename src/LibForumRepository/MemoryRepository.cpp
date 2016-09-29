#include <boost/locale.hpp>
#include <boost/regex/icu.hpp>

#include "Configuration.h"
#include "EntitySerialization.h"
#include "MemoryRepository.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StringHelpers.h"

using namespace Forum::Configuration;
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

void MemoryRepository::getUsers(std::ostream& output) const
{
    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByName();
                         writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                     });
}

void MemoryRepository::getUserByName(const std::string& name, std::ostream& output) const
{
    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& index = collection.usersByName();
                         auto it = index.find(name);
                         if (it == index.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "user", **it);
                         }
                     });
}

static const auto validUserNameRegex = boost::make_u32regex("^[[:alnum:]]+[ _-]*[[:alnum:]]+$");

static StatusCode validateUserName(const std::string& name, const ConfigConstRef& config)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    try
    {
        if ( ! boost::u32regex_match(name, validUserNameRegex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch(...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    if (countUTF8Characters(name) > config->user.maxNameLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }

    return StatusCode::OK;
}

StatusCode MemoryRepository::addNewUser(const std::string& name, std::ostream& output)
{
    auto validationCode = validateUserName(name, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return validationCode;
    }

    auto user = std::make_shared<User>();
    user->id() = generateUUIDString();
    user->name() = name;

    auto statusCode = StatusCode::OK;
    collection_.write([&](EntityCollection& collection)
                      {
                          auto byName = collection.usersByName();
                          if (byName.find(name) != byName.end())
                          {
                              statusCode = StatusCode::ALREADY_EXISTS;
                              return;
                          }
                          collection.users().insert(user);
                      });

    return statusCode;
}

StatusCode MemoryRepository::changeUserName(const IdType& id, const std::string& newName, std::ostream& output)
{
    auto validationCode = validateUserName(newName, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return validationCode;
    }

    auto statusCode = StatusCode::OK;
    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              statusCode = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                          if (indexByName.find(newName) != indexByName.end())
                          {
                              statusCode = StatusCode::ALREADY_EXISTS;
                              return;
                          }
                          collection.modifyUser((*it)->id(), [&newName](User& user)
                          {
                              user.name() = newName;
                          });
                      });

    return statusCode;
}
