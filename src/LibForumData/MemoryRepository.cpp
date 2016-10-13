#include <boost/locale.hpp>
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

MemoryRepository::MemoryRepository() : collection_(std::make_shared<EntityCollection>())
{
}

void MemoryRepository::addObserver(const ReadRepositoryObserverRef& observer)
{
    observers_.addObserver(observer);
}

void MemoryRepository::addObserver(const WriteRepositoryObserverRef& observer)
{
    observers_.addObserver(observer);
}

void MemoryRepository::removeObserver(const ReadRepositoryObserverRef& observer)
{
    observers_.removeObserver(observer);
}

void MemoryRepository::removeObserver(const WriteRepositoryObserverRef& observer)
{
    observers_.removeObserver(observer);
}

/**
 * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
 * The update is performed on the spot if a write lock is held or
 * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
 * Do not keep references to it outside of MemoryRepository methods
 */
struct Forum::Repository::PerformedByWithLastSeenUpdateGuard
{
    PerformedByWithLastSeenUpdateGuard(const MemoryRepository& repository) :
            repository_(const_cast<MemoryRepository&>(repository))
    {
    }

    ~PerformedByWithLastSeenUpdateGuard()
    {
        if (lastSeenUpdate_) lastSeenUpdate_();
    }

    /**
     * Get the current user that performs the action and optionally schedule the update of last seen
     */
    PerformedByType get(const EntityCollection& collection)
    {
        const auto& index = collection.usersById();
        auto it = index.find(Forum::Context::getCurrentUserId());
        if (it == index.end())
        {
            return AnonymousUser;
        }
        auto& result = **it;

        auto now = Forum::Context::getCurrentTime();

        if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
        {
            auto& userId = result.id();
            auto collection = &(repository_.collection_);
            lastSeenUpdate_ = [collection, now, userId]()
            {
                collection->write([&](EntityCollection& collection)
                                  {
                                      collection.modifyUser(userId, [&](User& user)
                                      {
                                          user.lastSeen() = now;
                                      });
                                  });
            };
        }
        return result;
    }

    /**
     * Get the current user that performs the action and optionally also perform the update of last seen
     * This method takes advantage if a write lock on the collection is already secured
     */
    PerformedByType getAndUpdate(EntityCollection& collection)
    {
        lastSeenUpdate_ = nullptr;
        const auto& index = collection.usersById();
        auto it = index.find(Forum::Context::getCurrentUserId());
        if (it == index.end())
        {
            return AnonymousUser;
        }
        auto& result = **it;

        auto now = Forum::Context::getCurrentTime();

        if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
        {
            collection.modifyUser(result.id(), [&](User& user)
            {
                user.lastSeen() = now;
            });
        }
        return result;
    }

private:
    MemoryRepository& repository_;
    std::function<void()> lastSeenUpdate_;
};

inline PerformedByWithLastSeenUpdateGuard preparePerformedBy(const MemoryRepository& repository)
{
    return PerformedByWithLastSeenUpdateGuard(repository);
}

void MemoryRepository::getUserCount(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto count = collection.usersById().size();
                         writeSingleValueSafeName(output, "count", count);

                         observers_.getUserCount(performedBy.get(collection));
                     });
}

void MemoryRepository::getUsersByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByName();
                         writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         observers_.getUsers(performedBy.get(collection));
                     });
}

void MemoryRepository::getUsersByCreated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByCreated();
                         writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         observers_.getUsers(performedBy.get(collection));
                     });
}

void MemoryRepository::getUsersByLastSeen(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& users = collection.usersByLastSeen();
                         writeSingleObjectSafeName(output, "users", Json::enumerate(users.begin(), users.end()));
                         observers_.getUsers(performedBy.get(collection));
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
                         observers_.getUserByName(performedBy.get(collection), name);
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
                          observers_.addNewUser(performedBy.getAndUpdate(collection), *user);

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
                          collection.modifyUser((*it)->id(), [&newName](User& user)
                          {
                              user.name() = newName;
                          });
                          observers_.changeUser(performedBy.getAndUpdate(collection), **it, User::ChangeType::Name);
                      });
}

void MemoryRepository::deleteUser(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
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
                          observers_.deleteUser(performedBy.getAndUpdate(collection), **it);
                          collection.deleteUser((*it)->id());
                      });
}
