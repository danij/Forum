#include "MemoryRepository.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

void MemoryRepository::getUsersByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByName();
                         writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         readEvents_.onGetUsers(createObserverContext(performedBy.get(collection)));
                     });
}

void MemoryRepository::getUsersByCreated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByCreated();
                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.rbegin(), users.rend()));
                         }
                         readEvents_.onGetUsers(createObserverContext(performedBy.get(collection)));
                     });
}
void MemoryRepository::getUsersByLastSeen(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByLastSeen();
                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "users", Json::enumerate(users.rbegin(), users.rend()));
                         }
                         readEvents_.onGetUsers(createObserverContext(performedBy.get(collection)));
                     });
}

void MemoryRepository::getUserById(const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

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
                         readEvents_.onGetUserById(createObserverContext(performedBy.get(collection)), id);
                     });
}

void MemoryRepository::getUserByName(const std::string& name, std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

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
                         readEvents_.onGetUserByName(createObserverContext(performedBy.get(collection)), name);
                     });
}

static StatusCode validateUserName(const std::string& name, const boost::u32regex regex, const ConfigConstRef& config)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    try
    {
        if ( ! boost::u32regex_match(name, regex, boost::match_flag_type::format_all))
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
    auto validationCode = validateUserName(name, validUserNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto user = std::make_shared<User>();
    user->id() = generateUUIDString();
    user->name() = name;
    user->created() = Context::getCurrentTime();

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexByName = collection.users().get<EntityCollection::UserCollectionByName>();
                          if (indexByName.find(name) != indexByName.end())
                          {
                              status = StatusCode::ALREADY_EXISTS;
                              return;
                          }
                          collection.users().insert(user);
                          writeEvents_.onAddNewUser(createObserverContext(*performedBy.getAndUpdate(collection)), *user);

                          status.addExtraSafeName("id", user->id());
                          status.addExtraSafeName("name", user->name());
                          status.addExtraSafeName("created", user->created());
                      });
}

void MemoryRepository::changeUserName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateUserName(newName, validUserNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
    }
    auto performedBy = preparePerformedBy();

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
                          writeEvents_.onChangeUser(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                    **it, User::ChangeType::Name);
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

    auto performedBy = preparePerformedBy();

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
                          writeEvents_.onDeleteUser(createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                          collection.deleteUser(it);
                      });
}
