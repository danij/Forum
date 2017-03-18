#include "MemoryRepositoryCommon.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

/**
 * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
 * The update is performed on the spot if a write lock is held or
 * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
 * Do not keep references to it outside of MemoryRepository methods
 */
PerformedByWithLastSeenUpdateGuard::~PerformedByWithLastSeenUpdateGuard()
{
    if (lastSeenUpdate_) lastSeenUpdate_();
}

PerformedByType PerformedByWithLastSeenUpdateGuard::get(const EntityCollection& collection, const MemoryStore& store)
{
    const auto& index = collection.usersById();
    auto it = index.find(Context::getCurrentUserId());
    if (it == index.end())
    {
        return *AnonymousUser;
    }
    auto& result = **it;

    auto now = Context::getCurrentTime();

    if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        auto& userId = result.id();
        auto& mutableCollection = const_cast<MemoryStore&>(store).collection;
        lastSeenUpdate_ = [&mutableCollection, now, &userId]()
        {
            mutableCollection.write([&](EntityCollection& collectionToModify)
                                    {
                                        collectionToModify.modifyUserById(userId, [ & ](User& user)
                                        {
                                            user.lastSeen() = now;
                                        });
                                    });
        };
    }
    return result;
}

UserRef PerformedByWithLastSeenUpdateGuard::getAndUpdate(EntityCollection& collection)
{
    lastSeenUpdate_ = nullptr;
    const auto& index = collection.users().get<EntityCollection::UserCollectionById>();
    auto it = index.find(Context::getCurrentUserId());
    if (it == index.end())
    {
        return AnonymousUser;
    }
    auto& result = *it;

    auto now = Context::getCurrentTime();

    if ((result->lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        collection.modifyUserById(result->id(), [&](User& user)
        {
            user.lastSeen() = now;
        });
    }
    return result;
}

static bool u32regex_match(const StringView& s,
                           const boost::u32regex& e,
                           boost::match_flag_type flags = boost::match_default)
{
    boost::match_results<const char*> m;
    return boost::BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<boost::mpl::int_<1> const*>(0));
}

StatusCode MemoryRepositoryBase::validateString(const StringView& string, 
                                                boost::optional<const boost::u32regex&> regex,
                                                EmptyStringValidation emptyValidation,
                                                boost::optional<int_fast32_t> minimumLength, 
                                                boost::optional<int_fast32_t> maximumLength)
{
    if (Context::skipValidations())
    {
        return StatusCode::OK;
    }

    if ((INVALID_PARAMETERS_FOR_EMPTY_STRING == emptyValidation) && string.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto nrCharacters = countUTF8Characters(string);
    if (maximumLength && (nrCharacters > *maximumLength))
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (minimumLength && (nrCharacters < *minimumLength))
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    try
    {
        if (regex && ! u32regex_match(string, *regex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch (...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    return StatusCode::OK;
}
