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

StatusCode MemoryRepository::getUsers(std::ostream& output, RetrieveUsersBy by) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
    {
        auto pageSize = getGlobalConfig()->user.maxUsersPerPage;
        auto& displayContext = Context::getDisplayContext();

        switch (by)
        {
        case RetrieveUsersBy::Name:
            writeEntitiesWithPagination(collection.usersByName(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](auto u) { return u; });
            break;
        case RetrieveUsersBy::Created:
            writeEntitiesWithPagination(collection.usersByCreated(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](auto u) { return u; });
            break;
        case RetrieveUsersBy::LastSeen:
            writeEntitiesWithPagination(collection.usersByLastSeen(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](auto u) { return u; });
            break;
        case RetrieveUsersBy::MessageCount:
            writeEntitiesWithPagination(collection.usersByMessageCount(), "users", output, displayContext.pageNumber, 
                pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, [](auto u) { return u; });
            break;
        }

        readEvents_.onGetUsers(createObserverContext(performedBy.get(collection)));
    });
    return StatusCode::OK;
}

StatusCode MemoryRepository::getUserById(const IdType& id, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& index = collection.usersById();
                         auto it = index.find(id);
                         if (it == index.end())
                         {
                             status = StatusCode::NOT_FOUND;
                             return;
                         }
                         else
                         {
                             status.disable();
                             writeSingleValueSafeName(output, "user", **it);
                         }
                         readEvents_.onGetUserById(createObserverContext(performedBy.get(collection)), id);
                     });
    return status;
}

StatusCode MemoryRepository::getUserByName(const std::string& name, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& index = collection.usersByName();
                         auto it = index.find(name);
                         if (it == index.end())
                         {
                             status = StatusCode::NOT_FOUND;
                             return;
                         }
                         else
                         {
                             status.disable();
                             writeSingleValueSafeName(output, "user", **it);
                         }
                         readEvents_.onGetUserByName(createObserverContext(performedBy.get(collection)), name);
                     });
    return status;
}

static StatusCode validateUserName(const std::string& name, const boost::u32regex regex, const ConfigConstRef& config)
{
    if (name.empty())
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

    return StatusCode::OK;
}

StatusCode MemoryRepository::addNewUser(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateUserName(name, validUserNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    auto user = std::make_shared<User>();
    user->id() = generateUUIDString();
    user->name() = name;
    updateCreated(*user);

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
    return status;
}

StatusCode MemoryRepository::changeUserName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateUserName(newName, validUserNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
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
    return status;
}

StatusCode MemoryRepository::changeUserInfo(const IdType& id, const std::string& newInfo, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto config = getGlobalConfig();
    
    auto nrCharacters = countUTF8Characters(newInfo);
    if (nrCharacters > config->user.maxInfoLength)
    {
        return status = StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->user.minInfoLength)
    {
        return status = StatusCode::VALUE_TOO_SHORT;
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
                          collection.modifyUser(it, [&newInfo](User& user)
                          {
                              user.info() = newInfo;
                          });
                          writeEvents_.onChangeUser(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                    **it, User::ChangeType::Info);
                      });
    return status;
}

StatusCode MemoryRepository::deleteUser(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
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
    return status;
}
