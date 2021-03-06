/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MemoryRepositoryDiscussionCategory.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"
#include "Logging.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryDiscussionCategory::MemoryRepositoryDiscussionCategory(MemoryStoreRef store,
                                                                       DiscussionCategoryAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategories(OutStream& output,
                                                                       RetrieveDiscussionCategoriesBy by) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, *store_);

        if ( ! (status = authorization_->getDiscussionCategories(currentUser)))
        {
            return;
        }

        BoolTemporaryChanger _(serializationSettings.hideLatestMessage, true);
        BoolTemporaryChanger __(serializationSettings.onlySendCategoryParentId, true);

        SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                             &currentUser, Context::getCurrentTime());

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionCategoriesBy::Name:
                writeArraySafeName(output, "categories", collection.categories().byName().begin(),
                                   collection.categories().byName().end(), restriction);
                status.disable();
                break;
            case RetrieveDiscussionCategoriesBy::MessageCount:
                //collection is sorted in greater order
                writeArraySafeName(output, "categories", collection.categories().byMessageCount().rbegin(),
                                   collection.categories().byMessageCount().rend(), restriction);
                status.disable();
                break;
            }
        }
        else
        {
            switch (by)
            {
            case RetrieveDiscussionCategoriesBy::Name:
                writeArraySafeName(output, "categories", collection.categories().byName().rbegin(),
                                   collection.categories().byName().rend(), restriction);
                status.disable();
                break;
            case RetrieveDiscussionCategoriesBy::MessageCount:
                //collection is sorted in greater order
                writeArraySafeName(output, "categories", collection.categories().byMessageCount().begin(),
                                   collection.categories().byMessageCount().end(), restriction);
                status.disable();
                break;
            }
        }

        readEvents().onGetDiscussionCategories(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategoriesFromRoot(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = authorization_->getDiscussionCategoriesFromRoot(currentUser)))
                          {
                              return;
                          }

                          auto index = collection.categories().byDisplayOrderRootPriority();
                          const auto indexBegin = index.begin();
                          const auto indexEnd = index.end();

                          auto indexRootEnd = indexBegin;
                          while ((indexRootEnd != indexEnd) && ((*indexRootEnd)->isRootCategory()))
                          {
                              ++indexRootEnd;
                          }

                          IntTemporaryChanger _(serializationSettings.showDiscussionCategoryChildrenRecursionLeft, 2);

                          status.disable();

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               &currentUser, Context::getCurrentTime());

                          writeArraySafeName(output, "categories", indexBegin, indexRootEnd, restriction);

                          readEvents().onGetRootDiscussionCategories(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategoryById(IdTypeRef id, OutStream& output) const
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& index = collection.categories().byId();
                          auto it = index.find(id);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          if ( ! (status = authorization_->getDiscussionCategoryById(currentUser, **it)))
                          {
                              return;
                          }

                          status.disable();
                          IntTemporaryChanger _(serializationSettings.showDiscussionCategoryChildrenRecursionLeft, 2);
                          BoolTemporaryChanger __(serializationSettings.keepDiscussionCategoryDetails, true);

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               &currentUser, Context::getCurrentTime());

                          writeSingleValueSafeName(output, "category", **it, restriction);

                          readEvents().onGetDiscussionCategory(createObserverContext(currentUser), **it);
                      });
    return status;
}


StatusCode MemoryRepositoryDiscussionCategory::addNewDiscussionCategory(StringView name, IdTypeRef parentId,
                                                                        OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->discussionCategory.minNameLength,
                                               config->discussionCategory.maxNameLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           DiscussionCategory::NameType nameString(name);

                           auto& indexByName = collection.categories().byName();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           auto& indexById = collection.categories().byId();
                           auto parentIt = indexById.find(parentId);
                           DiscussionCategoryPtr parent{};
                           if (parentIt != indexById.end())
                           {
                               parent = *parentIt;
                           }

                           if ( ! (status = authorization_->addNewDiscussionCategory(*currentUser, name, parent)))
                           {
                               return;
                           }

                           auto statusWithResource = addNewDiscussionCategory(collection, generateUniqueId(), name,
                                                                              parentId);
                           auto& category = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;


                           writeEvents().onAddNewDiscussionCategory(createObserverContext(*currentUser), *category);

                           auto setParentId = IdType::empty;
                           if (parent)
                           {
                               setParentId = parent->id();
                           }

                           status.writeNow([&](auto& writer)
                                           {
                                               JSON_WRITE_PROP(writer, "id", category->id());
                                               JSON_WRITE_PROP(writer, "name", category->name().string());
                                               JSON_WRITE_PROP(writer, "parentId", setParentId);
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionCategoryPtr>
    MemoryRepositoryDiscussionCategory::addNewDiscussionCategory(EntityCollection& collection, IdTypeRef id,
                                                                 StringView name, IdTypeRef parentId)
{
    DiscussionCategory::NameType nameString(name);

    auto& indexByName = collection.categories().byName();
    if (indexByName.find(nameString) != indexByName.end())
    {
        FORUM_LOG_ERROR << "A discussion category with this name already exists: " << name;
        return StatusCode::ALREADY_EXISTS;
    }

    auto& indexById = collection.categories().byId();

    //IdType id, Timestamp created, VisitDetails creationDetails
    auto category = collection.createDiscussionCategory(id, std::move(nameString), Context::getCurrentTime(),
                                                        { Context::getCurrentUserIpAddress() });

    if (parentId)
    {
        const auto parentIt = indexById.find(parentId);
        if (parentIt != indexById.end())
        {
            DiscussionCategoryPtr parent = *parentIt;

            parent->addChild(category);
            category->parent() = parent;
        }
    }

    collection.insertDiscussionCategory(category);

    return category;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryName(IdTypeRef id, StringView newName,
                                                                            OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->discussionCategory.minNameLength,
                                               config->discussionCategory.maxNameLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionCategory::NameType newNameString(newName);

                           auto& indexByName = collection.categories().byName();
                           if (indexByName.find(newNameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionCategoryName(*currentUser, **it, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionCategoryName(collection, id, std::move(newNameString)))) return;

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser), **it,
                                                                    DiscussionCategory::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryName(EntityCollection& collection,
                                                                            IdTypeRef id, StringView newName)
{
    return changeDiscussionCategoryName(collection, id,  DiscussionCategory::NameType(newName));
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryName(EntityCollection& collection,
                                                                            IdTypeRef id,
                                                                            DiscussionCategory::NameType&& newName)
{
    auto& indexById = collection.categories().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto& indexByName = collection.categories().byName();
    if (indexByName.find(newName) != indexByName.end())
    {
        FORUM_LOG_ERROR << "A discussion category with this name already exists: " << newName.string();
        return StatusCode::ALREADY_EXISTS;
    }

    const auto currentUser = getCurrentUser(collection);

    DiscussionCategoryPtr categoryPtr = *it;
    DiscussionCategory& category = *categoryPtr;

    category.updateName(std::move(newName));
    updateLastUpdated(category, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDescription(IdTypeRef id,
                                                                                   StringView newDescription,
                                                                                   OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    const auto maxLength = static_cast<int_fast32_t>(getGlobalConfig()->discussionCategory.maxDescriptionLength);
    if (countUTF8Characters(newDescription) > maxLength)
    {
        return status = StatusCode::VALUE_TOO_LONG;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& category = **it;

                           if ( ! (status = authorization_->changeDiscussionCategoryDescription(*currentUser, category, newDescription)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionCategoryDescription(collection, id, newDescription))) return;

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser),
                                                                    category, DiscussionCategory::ChangeType::Description);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDescription(EntityCollection& collection,
                                                                                   IdTypeRef id, StringView newDescription)
{
    auto& indexById = collection.categories().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    const auto currentUser = getCurrentUser(collection);

    DiscussionCategoryPtr categoryPtr = *it;
    DiscussionCategory& category = *categoryPtr;

    category.description() = toString(newDescription);
    updateLastUpdated(category, currentUser);

    return StatusCode::OK;
}
StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryParent(IdTypeRef id, IdTypeRef newParentId,
                                                                              OutStream& output)
{
    StatusWriter status(output);
    if (( ! id) || (id == newParentId))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionCategoryPtr categoryPtr = *it;
                           auto& category = **it;
                           auto newParentIt = indexById.find(newParentId);
                           DiscussionCategoryPtr newParentPtr{}; //might be empty

                           if (newParentIt != indexById.end())
                           {
                               newParentPtr = *newParentIt;

                               if (newParentPtr->hasAncestor(categoryPtr))
                               {
                                   status = StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED;
                                   return;
                               }
                           }

                           if ( ! (status = authorization_->changeDiscussionCategoryParent(*currentUser, **it, newParentPtr)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionCategoryParent(collection, id, newParentId))) return;

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser), category,
                                                                    DiscussionCategory::ChangeType::Parent);
                       });
    return status;
}

static void updateCategoryParent(DiscussionCategory& category, DiscussionCategoryPtr newParentPtr, UserPtr currentUser)
{
    auto oldParent = category.parent();

    category.updateParent(newParentPtr);
    updateLastUpdated(category, currentUser);

    if (oldParent)
    {
        oldParent->removeTotalsFromChild(category);
        oldParent->children().erase(&category);
    }

    if (newParentPtr)
    {
        newParentPtr->addTotalsFromChild(category);
        newParentPtr->children().insert(&category);
    }
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryParent(EntityCollection& collection,
                                                                              IdTypeRef id, IdTypeRef newParentId)
{
    auto& indexById = collection.categories().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionCategoryPtr categoryPtr = *it;
    DiscussionCategory& category = *categoryPtr;

    const auto newParentIt = indexById.find(newParentId);
    DiscussionCategoryPtr newParentPtr{}; //might be empty

    if (newParentIt != indexById.end())
    {
        newParentPtr = *newParentIt;
        //check that the new parent is not a child of the current category
        if (newParentPtr->hasAncestor(categoryPtr))
        {
            FORUM_LOG_ERROR << "Circular reference not allowed when assigning new parent "
                            << newParentId.toStringDashed() << " to discussion category "
                            << category.id().toStringDashed();
            return StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED;
        }
    }

    if (DiscussionCategoryPtr currentParent = category.parent())
    {
        if (currentParent->id() == newParentId)
        {
            return StatusCode::NO_EFFECT;
        }

        //remove the current category from it's parent child list
        currentParent->removeChild(categoryPtr);
    }

    const auto currentUser = getCurrentUser(collection);

    updateCategoryParent(category, newParentPtr, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDisplayOrder(IdTypeRef id,
                                                                                    int_fast16_t newDisplayOrder,
                                                                                    OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    if (newDisplayOrder < 0)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionCategoryDisplayOrder(*currentUser, **it, newDisplayOrder)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionCategoryDisplayOrder(collection, id, newDisplayOrder))) return;

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser), **it,
                                                                    DiscussionCategory::ChangeType::DisplayOrder);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDisplayOrder(EntityCollection& collection,
                                                                                    IdTypeRef id,
                                                                                    int_fast16_t newDisplayOrder)
{
    auto& indexById = collection.categories().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionCategoryPtr categoryPtr = *it;
    DiscussionCategory& category = *categoryPtr;
    const auto currentUser = getCurrentUser(collection);

    category.updateDisplayOrder(newDisplayOrder);
    updateLastUpdated(category, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::deleteDiscussionCategory(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->deleteDiscussionCategory(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the category is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionCategory(createObserverContext(*currentUser), **it);

                           status = deleteDiscussionCategory(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::deleteDiscussionCategory(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.categories().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionCategoryPtr category = *it;
    const auto currentUser = getCurrentUser(collection);

    std::vector<DiscussionCategoryPtr> childCategories;
    auto& children = category->children();
    std::copy(children.begin(), children.end(), std::back_inserter(childCategories));

    for (DiscussionCategoryPtr childCategory : childCategories)
    {
        updateCategoryParent(*childCategory, {}, currentUser);
    }

    collection.deleteDiscussionCategory(category);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::addDiscussionTagToCategory(IdTypeRef tagId, IdTypeRef categoryId,
                                                                          OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! categoryId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& categoryIndexById = collection.categories().byId();
                           auto categoryIt = categoryIndexById.find(categoryId);
                           if (categoryIt == categoryIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tag = **tagIt;
                           auto& category = **categoryIt;

                           if ( ! (status = authorization_->addDiscussionTagToCategory(*currentUser, tag, category)))
                           {
                               return;
                           }

                           if ( ! (status = addDiscussionTagToCategory(collection, tagId, categoryId))) return;

                           writeEvents().onAddDiscussionTagToCategory(createObserverContext(*currentUser),
                                                                      tag, category);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::addDiscussionTagToCategory(EntityCollection& collection,
                                                                          IdTypeRef tagId, IdTypeRef categoryId)
{
    auto& tagIndexById = collection.tags().byId();
    const auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << tagId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto& categoryIndexById = collection.categories().byId();
    const auto categoryIt = categoryIndexById.find(categoryId);
    if (categoryIt == categoryIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << categoryId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *tagIt;
    DiscussionTag& tag = *tagPtr;
    DiscussionCategoryPtr categoryPtr = *categoryIt;
    DiscussionCategory& category = *categoryPtr;

    //the number of categories associated to a tag is smaller than
    //the number of tags associated to a category, so search the category in the tag
    if ( ! tag.addCategory(categoryPtr))
    {
        return StatusCode::NO_EFFECT;
    }

    const auto currentUser = getCurrentUser(collection);

    category.addTag(tagPtr);
    updateLastUpdated(category, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::removeDiscussionTagFromCategory(IdTypeRef tagId,
                                                                               IdTypeRef categoryId,
                                                                               OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! categoryId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& categoryIndexById = collection.categories().byId();
                           auto categoryIt = categoryIndexById.find(categoryId);
                           if (categoryIt == categoryIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionTagPtr tagPtr = *tagIt;
                           DiscussionTag& tag = *tagPtr;
                           DiscussionCategoryPtr categoryPtr = *categoryIt;
                           DiscussionCategory& category = *categoryPtr;

                           if ( ! (status = authorization_->removeDiscussionTagFromCategory(*currentUser, tag, category)))
                           {
                               return;
                           }

                           if ( ! (status = removeDiscussionTagFromCategory(collection, tagId, categoryId))) return;

                           writeEvents().onRemoveDiscussionTagFromCategory(createObserverContext(*currentUser),
                                                                           tag, category);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::removeDiscussionTagFromCategory(EntityCollection& collection,
                                                                               IdTypeRef tagId, IdTypeRef categoryId)
{
    auto& tagIndexById = collection.tags().byId();
    const auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << tagId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto& categoryIndexById = collection.categories().byId();
    const auto categoryIt = categoryIndexById.find(categoryId);
    if (categoryIt == categoryIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion category: " << categoryId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *tagIt;
    DiscussionTag& tag = *tagPtr;
    DiscussionCategoryPtr categoryPtr = *categoryIt;
    DiscussionCategory& category = *categoryPtr;

    //the number of categories associated to a tag is smaller than
    //the number of tags associated to a category, so search the category in the tag
    if ( ! tag.removeCategory(categoryPtr))
    {
        return StatusCode::NO_EFFECT;
    }

    const auto currentUser = getCurrentUser(collection);

    category.removeTag(tagPtr);
    updateLastUpdated(category, currentUser);

    return StatusCode::OK;
}
