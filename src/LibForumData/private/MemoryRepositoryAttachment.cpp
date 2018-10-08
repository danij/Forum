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

#include "MemoryRepositoryAttachment.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "Logging.h"

#include <algorithm>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

static bool isValidAttachmentName(StringView input)
{
    if (input.empty())
    {
        return false;
    }

    if ((*input.begin() == ' ') || (*input.rbegin() == ' '))
    {
        return false; //do not allow leading of trailing white space
    }

    return std::all_of(input.begin(), input.end(), [](const char c)
    {
        return (c >= ' ') && (127 != c) && ('/' != c) && ('\\' != c);
    });
}

MemoryRepositoryAttachment::MemoryRepositoryAttachment(MemoryStoreRef store, AttachmentAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryAttachment::getAttachments(RetrieveAttachmentsBy by, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, *store_);

        if ( ! (status = authorization_->getAttachments(currentUser)))
        {
            return;
        }

        status.disable();

        const auto pageSize = getGlobalConfig()->attachment.maxAttachmentsPerPage;
        const auto& displayContext = Context::getDisplayContext();

        const SerializationRestriction restriction(collection.grantedPrivileges(), collection, currentUser.id(), 
                                                   Context::getCurrentTime());
        BoolTemporaryChanger _(serializationSettings.allowDisplayAttachmentIpAddress,
                restriction.isAllowed(ForumWidePrivilege::VIEW_ATTACHMENT_IP_ADDRESS));

        const auto ascending = displayContext.sortOrder == Context::SortOrder::Ascending;

        switch (by)
        {
        case RetrieveAttachmentsBy::Created:
            writeEntitiesWithPagination(collection.attachments().byCreated(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        case RetrieveAttachmentsBy::Name:
            writeEntitiesWithPagination(collection.attachments().byName(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        case RetrieveAttachmentsBy::Size:
            writeEntitiesWithPagination(collection.attachments().bySize(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        case RetrieveAttachmentsBy::Approval:
            writeEntitiesWithPagination(collection.attachments().byApproval(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        }

        readEvents().onGetAttachments(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryAttachment::getAttachmentsOfUser(IdTypeRef id, RetrieveAttachmentsBy by, 
                                                            OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, *store_);

        const auto& index = collection.users().byId();
        auto it = index.find(id);
        if (it == index.end())
        {
            status = StatusCode::NOT_FOUND;
            return;
        }
  
        auto& user = **it;

        if ( ! (status = authorization_->getAttachmentsOfUser(currentUser, user)))
        {
            return;
        }

        status.disable();

        const auto pageSize = getGlobalConfig()->attachment.maxAttachmentsPerPage;
        const auto& displayContext = Context::getDisplayContext();

        const SerializationRestriction restriction(collection.grantedPrivileges(), collection, currentUser.id(), 
                                                   Context::getCurrentTime());
        BoolTemporaryChanger _(serializationSettings.hideAttachmentCreatedBy, true);
        BoolTemporaryChanger __(serializationSettings.allowDisplayAttachmentIpAddress,
                restriction.isAllowed(ForumWidePrivilege::VIEW_ATTACHMENT_IP_ADDRESS));

        const auto ascending = displayContext.sortOrder == Context::SortOrder::Ascending;

        switch (by)
        {
        case RetrieveAttachmentsBy::Created:
            writeEntitiesWithPagination(user.attachments().byCreated(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        case RetrieveAttachmentsBy::Name:
            writeEntitiesWithPagination(user.attachments().byName(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        case RetrieveAttachmentsBy::Size:
            writeEntitiesWithPagination(user.attachments().bySize(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        case RetrieveAttachmentsBy::Approval:
            writeEntitiesWithPagination(user.attachments().byApproval(), "attachments", output, 
                displayContext.pageNumber, pageSize, ascending, restriction);
            break;
        }

        readEvents().onGetAttachments(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryAttachment::canGetAttachment(IdTypeRef id, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);
                      
                          const auto& indexById = collection.attachments().byId();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& attachment = **it;
                      
                          if ( ! (status = authorization_->canGetAttachment(currentUser, attachment)))
                          {
                              return;
                          }
                      
                          attachment.nrOfGetRequests() += 1;
                          readEvents().onGetAttachment(createObserverContext(currentUser), attachment);
                      });
    return status;
}

StatusCode MemoryRepositoryAttachment::addNewAttachment(StringView name, uint64_t size, OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->attachment.minNameLength, config->attachment.maxNameLength,
                                               isValidAttachmentName);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           if (currentUser->id() == anonymousUserId())
                           {
                               status = AuthorizationStatus::NOT_ALLOWED;
                               return;
                           }

                           if ( ! (status = authorization_->addNewAttachment(*currentUser, name, size)))
                           {
                               return;
                           }

                           const auto userQuota = currentUser->attachmentQuota()
                               ? currentUser->attachmentQuota()
                               : static_cast<uint64_t>(config->attachment.defaultUserQuota);

                           if ((currentUser->attachments().totalSize() + size) > userQuota)
                           {
                               status = StatusCode::QUOTA_EXCEEDED;
                               return;
                           }

                           const bool approved = AuthorizationStatus::OK
                               == authorization_->autoApproveAttachment(*currentUser);
                           
                           auto statusWithResource = addNewAttachment(collection, generateUniqueId(), name, size, 
                                                                      approved);
                           auto& attachment = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewAttachment(createObserverContext(*currentUser), *attachment);

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", attachment->id());
                                               writer << Json::propertySafeName("name", attachment->name().string());
                                               writer << Json::propertySafeName("created", attachment->created());
                                           });
                       });
    return status;
}

StatusWithResource<AttachmentPtr> MemoryRepositoryAttachment::addNewAttachment(EntityCollection& collection,
                                                                               IdTypeRef id, const StringView name, 
                                                                               const uint64_t size, const bool approved)
{
    auto currentUser = getCurrentUser(collection);

    const auto attachmentPtr = collection.createAttachment(id, Context::getCurrentTime(), 
                                                           { Context::getCurrentUserIpAddress() }, *currentUser, 
                                                           Attachment::NameType(name), size, approved);
    currentUser->attachments().add(attachmentPtr);

    collection.insertAttachment(attachmentPtr);

    return attachmentPtr;
}

StatusCode MemoryRepositoryAttachment::changeAttachmentName(IdTypeRef id, const StringView newName, OutStream& output)
{
    StatusWriter status(output);
    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->attachment.minNameLength, config->attachment.maxNameLength,
                                               isValidAttachmentName);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.attachments().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& attachment = **it;

                           if ( ! (status = authorization_->changeAttachmentName(*currentUser, attachment, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeAttachmentName(collection, id, newName))) return;

                           writeEvents().onChangeAttachment(createObserverContext(*currentUser), attachment, 
                                   Attachment::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryAttachment::changeAttachmentName(EntityCollection& collection, IdTypeRef id, 
                                                            const StringView newName)
{
    auto& indexById = collection.attachments().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find attachment: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    AttachmentPtr attachment = *it;
    attachment->updateName(Attachment::NameType(newName));

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAttachment::changeAttachmentApproval(IdTypeRef id, const bool newApproval, OutStream& output)
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.attachments().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           AttachmentPtr attachmentPtr = *it;
                           Attachment& attachment = *attachmentPtr;
                           
                           if ( ! (status = authorization_->changeAttachmentApproval(*currentUser, attachment, newApproval)))
                           {
                               return;
                           }

                           if ( ! (status = changeAttachmentApproval(collection, id, newApproval))) return;

                           const auto& write = writeEvents();
                           const auto observerContext = createObserverContext(*currentUser);

                           write.onChangeAttachment(observerContext, attachment, Attachment::ChangeType::Approval);
                       });
    return status;
}

StatusCode MemoryRepositoryAttachment::changeAttachmentApproval(EntityCollection& collection, IdTypeRef id, 
                                                                const bool newApproval)
{
    auto& indexById = collection.attachments().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find attachment: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    AttachmentPtr attachmentPtr = *it;
    Attachment& attachment = *attachmentPtr;

    if (attachment.approved() == newApproval)
    {
        return StatusCode::NO_EFFECT;
    }

    attachment.updateApproval(newApproval);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAttachment::deleteAttachment(IdTypeRef id, OutStream& output)
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
                           auto& indexById = collection.attachments().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           if ( ! (status = authorization_->deleteAttachment(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the attachment is not deleted before being passed to the observers
                           writeEvents().onDeleteAttachment(createObserverContext(*currentUser), **it);

                           status = deleteAttachment(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryAttachment::deleteAttachment(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.attachments().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find attachment: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    collection.deleteAttachment(*it);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryAttachment::addAttachmentToDiscussionThreadMessage(IdTypeRef attachmentId, 
                                                                              IdTypeRef messageId, OutStream& output)
{
    StatusWriter status(output);
    if (( ! attachmentId) || ( ! messageId))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           const auto& attachmentIndexById = collection.attachments().byId();
                           const auto attachmentIt = attachmentIndexById.find(attachmentId);
                           if (attachmentIt == attachmentIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           const auto& messageIndexById = collection.threadMessages().byId();
                           const auto messageIt = messageIndexById.find(messageId);
                           if (messageIt == messageIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->addAttachmentToDiscussionThreadMessage(*currentUser, **attachmentIt, **messageIt)))
                           {
                               return;
                           }

                           writeEvents().onAddAttachmentToDiscussionThreadMessage(createObserverContext(*currentUser), 
                                                                                  **attachmentIt, **messageIt);

                           auto statusWithResource = addAttachmentToDiscussionThreadMessage(collection, 
                                                                                            attachmentId, messageId);
                           if ( ! (status = statusWithResource.status))
                           {
                               return;
                           }
                           status.disable();

                           const SerializationRestriction restriction(collection.grantedPrivileges(), collection, 
                                   currentUser->id(), Context::getCurrentTime());

                           Json::JsonWriter writer(output);
                           writer.startObject();

                           writer.newPropertyWithSafeName("attachment");
                           serialize(writer, *statusWithResource.resource, restriction);
                            
                           writer.endObject();
                       });
    return status;    
}

StatusWithResource<AttachmentPtr> MemoryRepositoryAttachment::addAttachmentToDiscussionThreadMessage(
        EntityCollection& collection, IdTypeRef attachmentId, IdTypeRef messageId)
{
    auto& attachmentIndexById = collection.attachments().byId();
    const auto attachmentIt = attachmentIndexById.find(attachmentId);
    if (attachmentIt == attachmentIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find attachment: " << static_cast<std::string>(attachmentId);
        return StatusCode::NOT_FOUND;
    }

    auto& messageIndexById = collection.threadMessages().byId();
    const auto messageIt = messageIndexById.find(messageId);
    if (messageIt == messageIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(messageId);
        return StatusCode::NOT_FOUND;
    }

    AttachmentPtr attachmentPtr = *attachmentIt;
    DiscussionThreadMessagePtr messagePtr = *messageIt;

    if ( ! attachmentPtr->addMessage(messagePtr))
    {
        return StatusCode::ALREADY_EXISTS;
    }
    messagePtr->addAttachment(attachmentPtr);

    return attachmentPtr;
}

StatusCode MemoryRepositoryAttachment::removeAttachmentFromDiscussionThreadMessage(IdTypeRef attachmentId, 
                                                                                   IdTypeRef messageId, 
                                                                                   OutStream& output)
{
    StatusWriter status(output);
    if (( ! attachmentId) || ( ! messageId))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           const auto& attachmentIndexById = collection.attachments().byId();
                           const auto attachmentIt = attachmentIndexById.find(attachmentId);
                           if (attachmentIt == attachmentIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           const auto& messageIndexById = collection.threadMessages().byId();
                           const auto messageIt = messageIndexById.find(messageId);
                           if (messageIt == messageIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->removeAttachmentFromDiscussionThreadMessage(*currentUser, **attachmentIt, **messageIt)))
                           {
                               return;
                           }

                           writeEvents().onRemoveAttachmentFromDiscussionThreadMessage(
                                   createObserverContext(*currentUser), **attachmentIt, **messageIt);

                           status = removeAttachmentFromDiscussionThreadMessage(collection, attachmentId, messageId);
                       });
    return status;    
}

StatusCode MemoryRepositoryAttachment::removeAttachmentFromDiscussionThreadMessage(EntityCollection& collection,
                                                                                   IdTypeRef attachmentId, 
                                                                                   IdTypeRef messageId)
{
    auto& attachmentIndexById = collection.attachments().byId();
    const auto attachmentIt = attachmentIndexById.find(attachmentId);
    if (attachmentIt == attachmentIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find attachment: " << static_cast<std::string>(attachmentId);
        return StatusCode::NOT_FOUND;
    }

    auto& messageIndexById = collection.threadMessages().byId();
    const auto messageIt = messageIndexById.find(messageId);
    if (messageIt == messageIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(messageId);
        return StatusCode::NOT_FOUND;
    }

    AttachmentPtr attachmentPtr = *attachmentIt;
    DiscussionThreadMessagePtr messagePtr = *messageIt;

    if ( ! attachmentPtr->removeMessage(messagePtr))
    {
        return StatusCode::NO_EFFECT;
    }
    messagePtr->removeAttachment(attachmentPtr);
    
    return StatusCode::OK;
}
