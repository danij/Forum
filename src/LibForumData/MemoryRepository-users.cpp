#include <boost/regex/icu.hpp>

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "MemoryRepository.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

void MemoryRepository::getUsersByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByName();
                         writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         observers_.onGetUsers(performedBy.get(collection));
                     });
}

void MemoryRepository::getUsersByCreated(bool ascending, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByCreated();
                         if (ascending)
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.rbegin(), users.rend()));
                         }
                         observers_.onGetUsers(performedBy.get(collection));
                     });
}

void MemoryRepository::getUsersByCreatedAscending(std::ostream& output) const
{
    getUsersByCreated(true, output);
}

void MemoryRepository::getUsersByCreatedDescending(std::ostream& output) const
{
    getUsersByCreated(false, output);
}

void MemoryRepository::getUsersByLastSeen(bool ascending, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByLastSeen();
                         if (ascending)
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.rbegin(), users.rend()));
                         }
                         observers_.onGetUsers(performedBy.get(collection));
                     });
}

void MemoryRepository::getUsersByLastSeenAscending(std::ostream& output) const
{
    getUsersByLastSeen(true, output);
}

void MemoryRepository::getUsersByLastSeenDescending(std::ostream& output) const
{
    getUsersByLastSeen(false, output);
}

void MemoryRepository::getUserById(const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& index = collection.usersById();
                         auto it = index.find(id);
                         if (it == index.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "user", **it);
                         }
                         observers_.onGetUserById(performedBy.get(collection), id);
                     });
}

void MemoryRepository::getUserByName(const std::string& name, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

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
                         observers_.onGetUserByName(performedBy.get(collection), name);
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

    auto nrCharacters = countUTF8Characters(name);
    if (nrCharacters > config->user.maxNameLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->user.minNameLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    return StatusCode::OK;
}

void MemoryRepository::addNewUser(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateUserName(name, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto user = std::make_shared<User>();
    user->id() = generateUUIDString();
    user->name() = name;
    user->created() = Context::getCurrentTime();

    auto performedBy = preparePerformedBy(*this);

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                          if (indexByName.find(name) != indexByName.end())
                          {
                              status = StatusCode::ALREADY_EXISTS;
                              return;
                          }
                          collection.users().insert(user);
                          observers_.onAddNewUser(*performedBy.getAndUpdate(collection), *user);

                          status.addExtraSafeName("id", user->id());
                          status.addExtraSafeName("name", user->name());
                          status.addExtraSafeName("created", user->created());
                      });
}

void MemoryRepository::changeUserName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateUserName(newName, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
    }
    auto performedBy = preparePerformedBy(*this);

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                          if (indexByName.find(newName) != indexByName.end())
                          {
                              status = StatusCode::ALREADY_EXISTS;
                              return;
                          }
                          collection.modifyUser(it, [&newName](User& user)
                          {
                              user.name() = newName;
                          });
                          observers_.onChangeUser(*performedBy.getAndUpdate(collection), **it, User::ChangeType::Name);
                      });
}

void MemoryRepository::deleteUser(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    auto performedBy = preparePerformedBy(*this);

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.users().get<EntityCollection::UserCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          //make sure the user is not deleted before being passed to the observers
                          observers_.onDeleteUser(*performedBy.getAndUpdate(collection), **it);
                          collection.deleteUser(it);
                      });
}
