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

#include "EntityCollection.h"
#include "VectorWithFreeQueue.h"

#include "Configuration.h"
#include "StateHelpers.h"
#include "ContextProviders.h"
#include "Logging.h"

#include <future>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace Forum::Authorization;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;

/**
 * Used to prevent the individual removal of threads from a user's created threads collection when deleting a user
 */
static thread_local bool alsoDeleteThreadsFromUser = true;
/**
 * Used to prevent the individual removal of message from a user's created messages collection when deleting a user
 */
static thread_local bool alsoDeleteMessagesFromUser = true;
/**
 * Used to prevent the individual removal of private message from a user's sent private messages collection
 * when deleting a user
 */
static thread_local bool deletePrivateMessageFromSource = true;
/**
* Used to prevent the individual removal of private message from a user's received private messages collection
* when deleting a user
*/
static thread_local bool deletePrivateMessageFromDestination = true;
/**
 * Used to prevent the individual removal of attachments from a user's attachment collection when deleting a user
 */
static thread_local bool deleteAttachmentFromUser = true;

struct EntityCollection::Impl
{
    struct ManagedEntities
    {
        VectorWithFreeQueue<User> users;
        VectorWithFreeQueue<DiscussionThread> threads;
        VectorWithFreeQueue<DiscussionThreadMessage> threadMessages;
        VectorWithFreeQueue<DiscussionTag> tags;
        VectorWithFreeQueue<DiscussionCategory> categories;
        VectorWithFreeQueue<MessageComment> messageComments;
        VectorWithFreeQueue<PrivateMessage> privateMessages;
        VectorWithFreeQueue<Attachment> attachments;

    } managedEntities;

    UserCollection users_;
    DiscussionThreadCollectionWithHashedId threads_;
    DiscussionThreadMessageCollection threadMessages_;
    DiscussionTagCollection tags_;
    DiscussionCategoryCollection categories_;
    MessageCommentCollection messageComments_;
    PrivateMessageGlobalCollection privateMessages_;
    AttachmentCollection attachments_;

    GrantedPrivilegeStore grantedPrivileges_;

    bool batchInsertInProgress_{ false };

    boost::interprocess::file_mapping messagesFileMapping_;
    boost::interprocess::mapped_region messagesFileRegion_;
    const char* messagesFileStart_{ nullptr };
    size_t messagesFileSize_{};

    Impl(StringView messagesFile)
    {
        if (messagesFile.empty()) return;
        try
        {
            const auto mappingMode = boost::interprocess::read_only;
            boost::interprocess::file_mapping mapping(messagesFile.data(), mappingMode);
            boost::interprocess::mapped_region region(mapping, mappingMode);
            region.advise(boost::interprocess::mapped_region::advice_sequential);

            messagesFileStart_ = reinterpret_cast<const char*>(region.get_address());
            messagesFileSize_ = region.get_size();

            messagesFileMapping_ = std::move(mapping);
            messagesFileRegion_ = std::move(region);
        }
        catch (boost::interprocess::interprocess_exception& ex)
        {
            FORUM_LOG_ERROR << "Error mapping messages file: " << messagesFile << " (" << ex.what() << ')';
            std::abort();
        }
    }

    void insertUser(UserPtr user)
    {
        assert(user);
        users_.add(user);
    }

    void deleteUser(UserPtr userPtr)
    {
        assert(userPtr);
        if ( ! users_.remove(userPtr))
        {
            return;
        }
        User& user = *userPtr;

        for (DiscussionThreadMessagePtr message : user.votedMessages())
        {
            assert(message);
            message->removeVote(userPtr);
        }

        for (MessageCommentPtr comment : user.messageComments().byId())
        {
            assert(comment);
            if (comment->solved())
            {
                comment->parentMessage().decrementSolvedCommentsCount();
            }
            comment->parentMessage().removeComment(comment);
        }

        for (DiscussionThreadPtr thread : user.subscribedThreads().byId())
        {
            assert(thread);
            thread->subscribedUsers().erase(user.id());
        }

        {
            //no need to delete the message from the user as we're deleting the whole user anyway
            BoolTemporaryChanger changer(alsoDeleteMessagesFromUser, false);
            for (auto& message : user.threadMessages().byId())
            {
                assert(message);
                //Each discussion message holds a reference to the user that created it and the parent thread
                //As such, delete the discussion message before deleting the thread and the user
                deleteDiscussionThreadMessage(message);
            }
        }
        {
            //no need to delete the thread from the user as we're deleting the whole user anyway
            BoolTemporaryChanger changer(alsoDeleteThreadsFromUser, false);
            for (auto& thread : user.threads().byId())
            {
                assert(thread);
                //Each discussion thread holds a reference to the user that created it
                //As such, delete the discussion thread before deleting the user
                deleteDiscussionThread(thread, true);
            }
        }
        {
            //no need to delete the private message from the user's received collection
            //as we're deleting the whole user anyway
            BoolTemporaryChanger changer(deletePrivateMessageFromDestination, false);
            for (const auto messagePtr : user.receivedPrivateMessages().byId())
            {
                assert(messagePtr);
                deletePrivateMessage(messagePtr);
            }
        }
        {
            //no need to delete the private message from the user's sent collection
            //as we're deleting the whole user anyway
            BoolTemporaryChanger changer(deletePrivateMessageFromSource, false);
            for (const auto messagePtr : user.sentPrivateMessages().byId())
            {
                assert(messagePtr);
                deletePrivateMessage(messagePtr);
            }
        }
        {
            //no need to delete the attachment from the user's attachment collection
            //as we're deleting the whole user anyway
            BoolTemporaryChanger changer(deleteAttachmentFromUser, false);
            for (const auto attachmentPtr : user.attachments().byId())
            {
                assert(attachmentPtr);
                deleteAttachment(attachmentPtr);
            }
        }

        managedEntities.users.remove(userPtr.index());
    }

    void insertDiscussionThread(DiscussionThreadPtr thread)
    {
        assert(thread);
        threads_.add(thread);
    }

    void deleteDiscussionThread(DiscussionThreadPtr threadPtr, bool deleteMessages)
    {
        assert(threadPtr);

        if ( ! threads_.remove(threadPtr))
        {
            return;
        }
        DiscussionThread& thread = *threadPtr;

        thread.aboutToBeDeleted() = true;

        if (alsoDeleteThreadsFromUser)
        {
            thread.createdBy().threads().remove(threadPtr);
        }

        for (DiscussionCategoryPtr category : thread.categories())
        {
            assert(category);
            category->deleteDiscussionThread(threadPtr, deleteMessages, false);
        }

        for (DiscussionTagPtr tag : thread.tags())
        {
            assert(tag);
            tag->deleteDiscussionThread(threadPtr, deleteMessages);
        }

        for (auto& pair : thread.subscribedUsers())
        {
            UserPtr& user = pair.second;
            assert(user);
            user->subscribedThreads().remove(threadPtr);
        }

        if (deleteMessages)
        {
            //delete the messages at the end so as to not change the message count used in indexes
            for (auto& message : thread.messages().byId())
            {
                assert(message);
                //Each discussion message holds a reference to the user that created it and the parent thread
                //As such, delete the discussion message before deleting the thread
                deleteDiscussionThreadMessage(message);
            }
        }
        managedEntities.threads.remove(threadPtr.index());
    }

    void insertDiscussionThreadMessage(DiscussionThreadMessagePtr message)
    {
        assert(message);
        threadMessages_.add(message);
    }

    void deleteDiscussionThreadMessage(DiscussionThreadMessagePtr messagePtr)
    {
        assert(messagePtr);
        if ( ! threadMessages_.remove(messagePtr))
        {
            return;
        }
        DiscussionThreadMessage& message = *messagePtr;

        if (alsoDeleteMessagesFromUser)
        {
            message.createdBy().threadMessages().remove(messagePtr);
        }

        auto messageComments = message.comments();
        if (messageComments)
        {
            for (const MessageCommentPtr comment : messageComments->byId())
            {
                deleteMessageComment(comment);
            }
        }

        auto& upVotes = message.upVotes();
        if (upVotes)
        {
            for (const auto pair : *upVotes)
            {
                UserPtr user = pair.first;
                assert(user);
                user->removeVote(messagePtr);
            }
        }
        auto& downVotes = message.downVotes();
        if (downVotes)
        {
            for (const auto pair : *downVotes)
            {
                UserPtr user = pair.first;
                assert(user);
                user->removeVote(messagePtr);
            }
        }

        for (AttachmentPtr attachment : message.attachments())
        {
            attachment->messages().erase(messagePtr);
        }

        DiscussionThread& parentThread = *(message.parentThread());
        if (parentThread.aboutToBeDeleted())
        {
            managedEntities.threadMessages.remove(messagePtr.index());
            return;
        }

        parentThread.deleteDiscussionThreadMessage(messagePtr);
        parentThread.resetVisitorsSinceLastEdit();
        parentThread.latestVisibleChange() = Context::getCurrentTime();

        for (DiscussionTagPtr tag : parentThread.tags())
        {
            assert(tag);
            tag->updateMessageCount(-1);
        }

        for (DiscussionCategoryPtr category : parentThread.categories())
        {
            assert(category);
            category->updateMessageCount(message.parentThread(), -1);
        }

        managedEntities.threadMessages.remove(messagePtr.index());
    }

    void insertDiscussionTag(DiscussionTagPtr tag)
    {
        assert(tag);
        tags_.add(tag);
    }

    void deleteDiscussionTag(DiscussionTagPtr tagPtr)
    {
        assert(tagPtr);
        if ( ! tags_.remove(tagPtr))
        {
            return;
        }
        DiscussionTag& tag = *tagPtr;

        for (DiscussionCategoryPtr category : tag.categories())
        {
            assert(category);
            category->removeTag(tagPtr);
        }
        tag.threads().iterateThreads([tagPtr](DiscussionThreadPtr threadPtr)
        {
            assert(threadPtr);
            threadPtr->removeTag(tagPtr);
        });

        managedEntities.tags.remove(tagPtr.index());
    }

    void insertDiscussionCategory(DiscussionCategoryPtr category)
    {
        assert(category);
        categories_.add(category);
    }

    void deleteDiscussionCategory(DiscussionCategoryPtr categoryPtr)
    {
        assert(categoryPtr);
        if ( ! categories_.remove(categoryPtr))
        {
            return;
        }
        DiscussionCategory& category = *categoryPtr;

        for (DiscussionTagPtr tag : category.tags())
        {
            assert(tag);
            tag->removeCategory(categoryPtr);
        }

        category.threads().iterateThreads([categoryPtr](DiscussionThreadPtr threadPtr)
        {
            assert(threadPtr);
            threadPtr->removeCategory(categoryPtr);
        });

        auto parent = category.parent();
        if (parent)
        {
            parent->removeTotalsFromChild(category);
            parent->removeChild(categoryPtr);
        }

        managedEntities.categories.remove(categoryPtr.index());
    }

    void insertMessageComment(MessageCommentPtr comment)
    {
        assert(comment);
        messageComments_.add(comment);
    }

    void deleteMessageComment(MessageCommentPtr commentPtr)
    {
        assert(commentPtr);
        if ( ! messageComments_.remove(commentPtr))
        {
            return;
        }
        MessageComment& comment = *commentPtr;

        User& user = comment.createdBy();
        user.messageComments().remove(commentPtr);

        managedEntities.messageComments.remove(commentPtr.index());
    }

    void insertPrivateMessage(const PrivateMessagePtr messagePtr)
    {
        assert(messagePtr);
        privateMessages_.add(messagePtr);
    }

    void deletePrivateMessage(PrivateMessagePtr messagePtr)
    {
        assert(messagePtr);
        if ( ! privateMessages_.remove(messagePtr))
        {
            return;
        }
        PrivateMessage& message = *messagePtr;

        if (deletePrivateMessageFromSource)
        {
            User& sourceUser = message.source();
            sourceUser.sentPrivateMessages().remove(messagePtr);
        }
        if (deletePrivateMessageFromDestination)
        {
            User& destinationUser = message.destination();
            destinationUser.receivedPrivateMessages().remove(messagePtr);
        }
        managedEntities.privateMessages.remove(messagePtr.index());
    }

    void insertAttachment(const AttachmentPtr attachmentPtr)
    {
        assert(attachmentPtr);
        attachments_.add(attachmentPtr);
    }

    void deleteAttachment(AttachmentPtr attachmentPtr)
    {
        assert(attachmentPtr);
        if ( ! attachments_.remove(attachmentPtr))
        {
            return;
        }
        Attachment& attachment = *attachmentPtr;

        if (deleteAttachmentFromUser)
        {
            User& user = attachment.createdBy();
            user.attachments().remove(attachmentPtr);
        }
        for (DiscussionThreadMessagePtr messagePtr : attachment.messages())
        {
            assert(messagePtr);
            messagePtr->removeAttachment(attachmentPtr);

        }
        managedEntities.attachments.remove(attachmentPtr.index());
    }

    void setEventListeners()
    {
        User::changeNotifications().onPrepareUpdateAuth         = [this](auto& user) { this->onPrepareUpdateUserAuth(user); };
        User::changeNotifications().onPrepareUpdateAuth         = [this](auto& user) { this->onPrepareUpdateUserAuth(user); };
        User::changeNotifications().onPrepareUpdateName         = [this](auto& user) { this->onPrepareUpdateUserName(user); };
        User::changeNotifications().onPrepareUpdateLastSeen     = [this](auto& user) { this->onPrepareUpdateUserLastSeen(user); };
        User::changeNotifications().onPrepareUpdateThreadCount  = [this](auto& user) { this->onPrepareUpdateUserThreadCount(user); };
        User::changeNotifications().onPrepareUpdateMessageCount = [this](auto& user) { this->onPrepareUpdateUserMessageCount(user); };

        User::changeNotifications().onUpdateAuth         = [this](auto& user) { this->onUpdateUserAuth(user); };
        User::changeNotifications().onUpdateAuth         = [this](auto& user) { this->onUpdateUserAuth(user); };
        User::changeNotifications().onUpdateName         = [this](auto& user) { this->onUpdateUserName(user); };
        User::changeNotifications().onUpdateLastSeen     = [this](auto& user) { this->onUpdateUserLastSeen(user); };
        User::changeNotifications().onUpdateThreadCount  = [this](auto& user) { this->onUpdateUserThreadCount(user); };
        User::changeNotifications().onUpdateMessageCount = [this](auto& user) { this->onUpdateUserMessageCount(user); };

        DiscussionThread::changeNotifications().onPrepareUpdateName                 = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadName(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdateLastUpdated          = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadLastUpdated(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdateLatestMessageCreated = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadLatestMessageCreated(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdateMessageCount         = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadMessageCount(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdatePinDisplayOrder      = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadPinDisplayOrder(thread); };

        DiscussionThread::changeNotifications().onUpdateName                 = [this](auto& thread) { this->onUpdateDiscussionThreadName(thread); };
        DiscussionThread::changeNotifications().onUpdateLastUpdated          = [this](auto& thread) { this->onUpdateDiscussionThreadLastUpdated(thread); };
        DiscussionThread::changeNotifications().onUpdateLatestMessageCreated = [this](auto& thread) { this->onUpdateDiscussionThreadLatestMessageCreated(thread); };
        DiscussionThread::changeNotifications().onUpdateMessageCount         = [this](auto& thread) { this->onUpdateDiscussionThreadMessageCount(thread); };
        DiscussionThread::changeNotifications().onUpdatePinDisplayOrder      = [this](auto& thread) { this->onUpdateDiscussionThreadPinDisplayOrder(thread); };

        DiscussionTag::changeNotifications().onPrepareUpdateName         = [this](auto& tag) { this->onPrepareUpdateDiscussionTagName(tag); };
        DiscussionTag::changeNotifications().onPrepareUpdateThreadCount  = [this](auto& tag) { this->onPrepareUpdateDiscussionTagThreadCount(tag); };
        DiscussionTag::changeNotifications().onPrepareUpdateMessageCount = [this](auto& tag) { this->onPrepareUpdateDiscussionTagMessageCount(tag); };

        DiscussionTag::changeNotifications().onUpdateName         = [this](auto& tag) { this->onUpdateDiscussionTagName(tag); };
        DiscussionTag::changeNotifications().onUpdateThreadCount  = [this](auto& tag) { this->onUpdateDiscussionTagThreadCount(tag); };
        DiscussionTag::changeNotifications().onUpdateMessageCount = [this](auto& tag) { this->onUpdateDiscussionTagMessageCount(tag); };

        DiscussionCategory::changeNotifications().onPrepareUpdateName         = [this](auto& category) { this->onPrepareUpdateDiscussionCategoryName(category); };
        DiscussionCategory::changeNotifications().onPrepareUpdateMessageCount = [this](auto& category) { this->onPrepareUpdateDiscussionCategoryMessageCount(category); };
        DiscussionCategory::changeNotifications().onPrepareUpdateDisplayOrder = [this](auto& category) { this->onPrepareUpdateDiscussionCategoryDisplayOrder(category); };

        DiscussionCategory::changeNotifications().onUpdateName         = [this](auto& category) { this->onUpdateDiscussionCategoryName(category); };
        DiscussionCategory::changeNotifications().onUpdateMessageCount = [this](auto& category) { this->onUpdateDiscussionCategoryMessageCount(category); };
        DiscussionCategory::changeNotifications().onUpdateDisplayOrder = [this](auto& category) { this->onUpdateDiscussionCategoryDisplayOrder(category); };

        Attachment::changeNotifications().onPrepareUpdateName     = [this](auto& attachment) { this->onPrepareUpdateAttachmentName(attachment); };
        Attachment::changeNotifications().onPrepareUpdateApproval = [this](auto& attachment) { this->onPrepareUpdateAttachmentApproval(attachment); };
    }

    void onPrepareUpdateUserAuth        (const User& user) { users_.prepareUpdateAuth(user.pointer()); }
    void onPrepareUpdateUserName        (const User& user) { users_.prepareUpdateName(user.pointer()); }
    void onPrepareUpdateUserLastSeen    (const User& user) { if ( ! batchInsertInProgress_) users_.prepareUpdateLastSeen(user.pointer()); }
    void onPrepareUpdateUserThreadCount (const User& user) { if ( ! batchInsertInProgress_) users_.prepareUpdateThreadCount(user.pointer()); }
    void onPrepareUpdateUserMessageCount(const User& user) { if ( ! batchInsertInProgress_) users_.prepareUpdateMessageCount(user.pointer()); }

    void onUpdateUserAuth        (const User& user) { users_.updateAuth(user.pointer()); }
    void onUpdateUserName        (const User& user) { users_.updateName(user.pointer()); }
    void onUpdateUserLastSeen    (const User& user) { if ( ! batchInsertInProgress_) users_.updateLastSeen(user.pointer()); }
    void onUpdateUserThreadCount (const User& user) { if ( ! batchInsertInProgress_) users_.updateThreadCount(user.pointer()); }
    void onUpdateUserMessageCount(const User& user) { if ( ! batchInsertInProgress_) users_.updateMessageCount(user.pointer()); }

    void discussionThreadAction(const DiscussionThread& constThread,
                                void (IDiscussionThreadCollection::*fn)(DiscussionThreadPtr))
    {
        auto& thread = const_cast<DiscussionThread&>(constThread);
        const DiscussionThreadPtr threadPtr = thread.pointer();

        (threads_.*fn)(threadPtr);
        (thread.createdBy().threads().*fn)(threadPtr);

        for (auto& pair : thread.subscribedUsers())
        {
            UserPtr& user = pair.second;
            assert(user);
            (user->subscribedThreads().*fn)(threadPtr);
        }

        for (DiscussionTagPtr tag : thread.tags())
        {
            assert(tag);
            (tag->threads().*fn)(threadPtr);
        }

        for (DiscussionCategoryPtr category : thread.categories())
        {
            assert(category);
            (category->threads().*fn)(threadPtr);
        }
    }

    void attachmentAction(const Attachment& constAttachment, void (AttachmentCollection::*fn)(AttachmentPtr))
    {
        auto& attachment = const_cast<Attachment&>(constAttachment);
        const AttachmentPtr attachmentPtr = attachment.pointer();

        (attachments_.*fn)(attachmentPtr);
        (attachment.createdBy().attachments().*fn)(attachmentPtr);
    }

    void onPrepareUpdateDiscussionThreadName(const DiscussionThread& thread)                 { discussionThreadAction(thread, &IDiscussionThreadCollection::prepareUpdateName); }
    void onPrepareUpdateDiscussionThreadLastUpdated(const DiscussionThread& thread)          { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::prepareUpdateLastUpdated); }
    void onPrepareUpdateDiscussionThreadLatestMessageCreated(const DiscussionThread& thread) { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::prepareUpdateLatestMessageCreated); }
    void onPrepareUpdateDiscussionThreadMessageCount(const DiscussionThread& thread)         { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::prepareUpdateMessageCount); }
    void onPrepareUpdateDiscussionThreadPinDisplayOrder(const DiscussionThread& thread)      { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::prepareUpdatePinDisplayOrder); }

    void onUpdateDiscussionThreadName(const DiscussionThread& thread)                 { discussionThreadAction(thread, &IDiscussionThreadCollection::updateName); }
    void onUpdateDiscussionThreadLastUpdated(const DiscussionThread& thread)          { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::updateLastUpdated); }
    void onUpdateDiscussionThreadLatestMessageCreated(const DiscussionThread& thread) { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::updateLatestMessageCreated); }
    void onUpdateDiscussionThreadMessageCount(const DiscussionThread& thread)         { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::updateMessageCount); }
    void onUpdateDiscussionThreadPinDisplayOrder(const DiscussionThread& thread)      { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &IDiscussionThreadCollection::updatePinDisplayOrder); }

    void onPrepareUpdateDiscussionTagName        (const DiscussionTag& tag) { tags_.prepareUpdateName(tag.pointer()); }
    void onPrepareUpdateDiscussionTagThreadCount (const DiscussionTag& tag) { if ( ! batchInsertInProgress_) tags_.prepareUpdateThreadCount(tag.pointer());}
    void onPrepareUpdateDiscussionTagMessageCount(const DiscussionTag& tag) { if ( ! batchInsertInProgress_) tags_.prepareUpdateMessageCount(tag.pointer()); }

    void onUpdateDiscussionTagName        (const DiscussionTag& tag) { tags_.updateName(tag.pointer()); }
    void onUpdateDiscussionTagThreadCount (const DiscussionTag& tag) { if ( ! batchInsertInProgress_) tags_.updateThreadCount(tag.pointer()); }
    void onUpdateDiscussionTagMessageCount(const DiscussionTag& tag) { if ( ! batchInsertInProgress_) tags_.updateMessageCount(tag.pointer()); }

    void onPrepareUpdateDiscussionCategoryName        (const DiscussionCategory& category) { categories_.prepareUpdateName(category.pointer()); }
    void onPrepareUpdateDiscussionCategoryMessageCount(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.prepareUpdateMessageCount(category.pointer()); }
    void onPrepareUpdateDiscussionCategoryDisplayOrder(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.prepareUpdateDisplayOrderRootPriority(category.pointer()); }

    void onUpdateDiscussionCategoryName        (const DiscussionCategory& category) { categories_.updateName(category.pointer()); }
    void onUpdateDiscussionCategoryMessageCount(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.updateMessageCount(category.pointer()); }
    void onUpdateDiscussionCategoryDisplayOrder(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.updateDisplayOrderRootPriority(category.pointer()); }

    void onPrepareUpdateAttachmentName    (const Attachment& attachment) { if ( ! batchInsertInProgress_) attachmentAction(attachment, &AttachmentCollection::prepareUpdateName); }
    void onPrepareUpdateAttachmentApproval(const Attachment& attachment) { if ( ! batchInsertInProgress_) attachmentAction(attachment, &AttachmentCollection::prepareUpdateApproval); }

    void toggleBatchInsert(const bool activate)
    {
        if ( ! activate) assert(batchInsertInProgress_);

        if (activate)
        {
            Context::setBatchInsertInProgres(batchInsertInProgress_ = activate);
            return;
        }

        users_.stopBatchInsert();

        std::future<void> futures[] =
        {
            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->threadMessages().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                this->threads_.iterateThreads([](DiscussionThreadPtr threadPtr)
                {
                    threadPtr->messages().stopBatchInsert();
                });
            }),

            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->threads().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->subscribedThreads().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->attachments().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (DiscussionTagPtr tag : this->tags_.byId())
                {
                    tag->threads().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (DiscussionCategoryPtr category : this->categories_.byId())
                {
                    category->stopBatchInsert();
                }
            })
        };

        for (auto& future : futures)
        {
            future.get();
        }

        this->threads_.stopBatchInsert();
        this->threadMessages_.stopBatchInsert();
        this->tags_.stopBatchInsert();
        this->categories_.stopBatchInsert();
        this->attachments_.stopBatchInsert();

        Context::setBatchInsertInProgres(batchInsertInProgress_ = activate);
    }

    StringView getMessageContentPointer(size_t offset, size_t size)
    {
        if (nullptr == messagesFileStart_)
        {
            return{};
        }

        const auto requiredSize = offset + size;
        if (requiredSize > messagesFileSize_)
        {
            return{};
        }

        return{ messagesFileStart_ + offset, size };
    }
};

static UserPtr anonymousUser_;
static IdType anonymousUserId_;

UserPtr Forum::Entities::anonymousUser()
{
    return anonymousUser_;
}

IdType Forum::Entities::anonymousUserId()
{
    return anonymousUserId_;
}

static void loadDefaultPrivilegeValues(ForumWidePrivilegeStore& store)
{
    const auto config = getGlobalConfig();
    const auto& defaultPrivileges = config->defaultPrivileges;

    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW,                       defaultPrivileges.threadMessage.view);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_UNAPPROVED,            defaultPrivileges.threadMessage.viewUnapproved);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_REQUIRED_PRIVILEGES,   defaultPrivileges.threadMessage.viewRequiredPrivileges);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_ASSIGNED_PRIVILEGES,   defaultPrivileges.threadMessage.viewAssignedPrivileges);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER,          defaultPrivileges.threadMessage.viewCreatorUser);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_IP_ADDRESS,            defaultPrivileges.threadMessage.viewIpAddress);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_VOTES,                 defaultPrivileges.threadMessage.viewVotes);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_ATTACHMENT,            defaultPrivileges.threadMessage.viewAttachment);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::VIEW_UNAPPROVED_ATTACHMENT, defaultPrivileges.threadMessage.viewUnapprovedAttachment);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::UP_VOTE,                    defaultPrivileges.threadMessage.upVote);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::DOWN_VOTE,                  defaultPrivileges.threadMessage.downVote);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::RESET_VOTE,                 defaultPrivileges.threadMessage.resetVote);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::ADD_COMMENT,                defaultPrivileges.threadMessage.addComment);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::SET_COMMENT_TO_SOLVED,      defaultPrivileges.threadMessage.setCommentToSolved);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS,       defaultPrivileges.threadMessage.getMessageComments);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::CHANGE_CONTENT,             defaultPrivileges.threadMessage.changeContent);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::CHANGE_APPROVAL,            defaultPrivileges.threadMessage.changeApproval);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::DELETE,                     defaultPrivileges.threadMessage.deleteThreadMessage);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::MOVE,                       defaultPrivileges.threadMessage.move);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::ADD_ATTACHMENT,             defaultPrivileges.threadMessage.addAttachment);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::REMOVE_ATTACHMENT,          defaultPrivileges.threadMessage.removeAttachment);
    store.setDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE,           defaultPrivileges.threadMessage.adjustPrivilege);

    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::VIEW,                     defaultPrivileges.thread.view);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::VIEW_UNAPPROVED,          defaultPrivileges.thread.viewUnapproved);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::VIEW_REQUIRED_PRIVILEGES, defaultPrivileges.thread.viewRequiredPrivileges);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::VIEW_ASSIGNED_PRIVILEGES, defaultPrivileges.thread.viewAssignedPrivileges);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::GET_SUBSCRIBED_USERS,     defaultPrivileges.thread.getSubscribedUsers);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::SUBSCRIBE,                defaultPrivileges.thread.subscribe);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::UNSUBSCRIBE,              defaultPrivileges.thread.unsubscribe);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::ADD_MESSAGE,              defaultPrivileges.thread.addMessage);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::AUTO_APPROVE_MESSAGE,     defaultPrivileges.thread.autoApproveMessage);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::CHANGE_NAME,              defaultPrivileges.thread.changeName);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::CHANGE_PIN_DISPLAY_ORDER, defaultPrivileges.thread.changePinDisplayOrder);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::CHANGE_APPROVAL,          defaultPrivileges.thread.changeApproval);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::ADD_TAG,                  defaultPrivileges.thread.addTag);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::REMOVE_TAG,               defaultPrivileges.thread.removeTag);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::DELETE,                   defaultPrivileges.thread.deleteThread);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::MERGE,                    defaultPrivileges.thread.merge);
    store.setDiscussionThreadPrivilege(DiscussionThreadPrivilege::ADJUST_PRIVILEGE,         defaultPrivileges.thread.adjustPrivilege);

    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::VIEW,                     defaultPrivileges.tag.view);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::VIEW_REQUIRED_PRIVILEGES, defaultPrivileges.tag.viewRequiredPrivileges);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::VIEW_ASSIGNED_PRIVILEGES, defaultPrivileges.tag.viewAssignedPrivileges);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::GET_DISCUSSION_THREADS,   defaultPrivileges.tag.getDiscussionThreads);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::CHANGE_NAME,              defaultPrivileges.tag.changeName);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::CHANGE_UIBLOB,            defaultPrivileges.tag.changeUiblob);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::DELETE,                   defaultPrivileges.tag.deleteTag);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::MERGE,                    defaultPrivileges.tag.merge);
    store.setDiscussionTagPrivilege(DiscussionTagPrivilege::ADJUST_PRIVILEGE,         defaultPrivileges.tag.adjustPrivilege);

    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::VIEW,                     defaultPrivileges.category.view);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::VIEW_REQUIRED_PRIVILEGES, defaultPrivileges.category.viewRequiredPrivileges);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::VIEW_ASSIGNED_PRIVILEGES, defaultPrivileges.category.viewAssignedPrivileges);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::GET_DISCUSSION_THREADS,   defaultPrivileges.category.getDiscussionThreads);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::CHANGE_NAME,              defaultPrivileges.category.changeName);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::CHANGE_DESCRIPTION,       defaultPrivileges.category.changeDescription);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::CHANGE_PARENT,            defaultPrivileges.category.changeParent);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER,      defaultPrivileges.category.changeDisplayorder);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::ADD_TAG,                  defaultPrivileges.category.addTag);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::REMOVE_TAG,               defaultPrivileges.category.removeTag);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::DELETE,                   defaultPrivileges.category.deleteCategory);
    store.setDiscussionCategoryPrivilege(DiscussionCategoryPrivilege::ADJUST_PRIVILEGE,         defaultPrivileges.category.adjustPrivilege);

    store.setForumWidePrivilege(ForumWidePrivilege::ADD_USER,                                  defaultPrivileges.forumWide.addUser);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ENTITIES_COUNT,                        defaultPrivileges.forumWide.getEntitiesCount);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_VERSION,                               defaultPrivileges.forumWide.getVersion);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ALL_USERS,                             defaultPrivileges.forumWide.getAllUsers);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_USER_INFO,                             defaultPrivileges.forumWide.getUserInfo);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_DISCUSSION_THREADS_OF_USER,            defaultPrivileges.forumWide.getDiscussionThreadsOfUser);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_DISCUSSION_THREAD_MESSAGES_OF_USER,    defaultPrivileges.forumWide.getDiscussionThreadMessagesOfUser);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER, defaultPrivileges.forumWide.getSubscribedDiscussionThreadsOfUser);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ALL_DISCUSSION_CATEGORIES,             defaultPrivileges.forumWide.getAllDiscussionCategories);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_DISCUSSION_CATEGORIES_FROM_ROOT,       defaultPrivileges.forumWide.getDiscussionCategoriesFromRoot);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ALL_DISCUSSION_TAGS,                   defaultPrivileges.forumWide.getAllDiscussionTags);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ALL_DISCUSSION_THREADS,                defaultPrivileges.forumWide.getAllDiscussionThreads);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ALL_MESSAGE_COMMENTS,                  defaultPrivileges.forumWide.getAllMessageComments);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_MESSAGE_COMMENTS_OF_USER,              defaultPrivileges.forumWide.getMessageCommentsOfUser);
    store.setForumWidePrivilege(ForumWidePrivilege::ADD_DISCUSSION_CATEGORY,                   defaultPrivileges.forumWide.addDiscussionCategory);
    store.setForumWidePrivilege(ForumWidePrivilege::ADD_DISCUSSION_TAG,                        defaultPrivileges.forumWide.addDiscussionTag);
    store.setForumWidePrivilege(ForumWidePrivilege::ADD_DISCUSSION_THREAD,                     defaultPrivileges.forumWide.addDiscussionThread);
    store.setForumWidePrivilege(ForumWidePrivilege::AUTO_APPROVE_DISCUSSION_THREAD,            defaultPrivileges.forumWide.autoApproveDiscussionThread);
    store.setForumWidePrivilege(ForumWidePrivilege::SEND_PRIVATE_MESSAGE,                      defaultPrivileges.forumWide.sendPrivateMessage);
    store.setForumWidePrivilege(ForumWidePrivilege::VIEW_PRIVATE_MESSAGE_IP_ADDRESS,           defaultPrivileges.forumWide.viewPrivateMessageIpAddress);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_OWN_USER_NAME,                      defaultPrivileges.forumWide.changeOwnUserName);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_OWN_USER_INFO,                      defaultPrivileges.forumWide.changeOwnUserInfo);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_USER_NAME,                      defaultPrivileges.forumWide.changeAnyUserName);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_USER_INFO,                      defaultPrivileges.forumWide.changeAnyUserInfo);
    store.setForumWidePrivilege(ForumWidePrivilege::DELETE_ANY_USER,                           defaultPrivileges.forumWide.deleteAnyUser);
    store.setForumWidePrivilege(ForumWidePrivilege::VIEW_FORUM_WIDE_REQUIRED_PRIVILEGES,       defaultPrivileges.forumWide.viewForumWideRequiredPrivileges);
    store.setForumWidePrivilege(ForumWidePrivilege::VIEW_FORUM_WIDE_ASSIGNED_PRIVILEGES,       defaultPrivileges.forumWide.viewForumWideAssignedPrivileges);
    store.setForumWidePrivilege(ForumWidePrivilege::VIEW_USER_ASSIGNED_PRIVILEGES,             defaultPrivileges.forumWide.viewUserAssignedPrivileges);
    store.setForumWidePrivilege(ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE,               defaultPrivileges.forumWide.adjustForumWidePrivilege);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_OWN_USER_TITLE,                     defaultPrivileges.forumWide.changeOwnUserTitle);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_USER_TITLE,                     defaultPrivileges.forumWide.changeAnyUserTitle);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_OWN_USER_SIGNATURE,                 defaultPrivileges.forumWide.changeOwnUserSignature);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_USER_SIGNATURE,                 defaultPrivileges.forumWide.changeAnyUserSignature);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_OWN_USER_LOGO,                      defaultPrivileges.forumWide.changeOwnUserLogo);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_USER_LOGO,                      defaultPrivileges.forumWide.changeAnyUserLogo);
    store.setForumWidePrivilege(ForumWidePrivilege::DELETE_OWN_USER_LOGO,                      defaultPrivileges.forumWide.deleteOwnUserLogo);
    store.setForumWidePrivilege(ForumWidePrivilege::DELETE_ANY_USER_LOGO,                      defaultPrivileges.forumWide.deleteAnyUserLogo);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_USER_ATTACHMENT_QUOTA,              defaultPrivileges.forumWide.changeAnyUserAttachmentQuota);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ALL_ATTACHMENTS,                       defaultPrivileges.forumWide.getAllAttachments);
    store.setForumWidePrivilege(ForumWidePrivilege::GET_ATTACHMENTS_OF_USER,                   defaultPrivileges.forumWide.getAttachmentsOfUser);
    store.setForumWidePrivilege(ForumWidePrivilege::VIEW_ATTACHMENT_IP_ADDRESS,                defaultPrivileges.forumWide.viewAttachmentIpAddress);
    store.setForumWidePrivilege(ForumWidePrivilege::CREATE_ATTACHMENT,                         defaultPrivileges.forumWide.createAttachment);
    store.setForumWidePrivilege(ForumWidePrivilege::AUTO_APPROVE_ATTACHMENT,                   defaultPrivileges.forumWide.autoApproveAttachment);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_ATTACHMENT_NAME,                defaultPrivileges.forumWide.changeAnyAttachmentName);
    store.setForumWidePrivilege(ForumWidePrivilege::CHANGE_ANY_ATTACHMENT_APPROVAL,            defaultPrivileges.forumWide.changeAnyAttachmentApproval);
    store.setForumWidePrivilege(ForumWidePrivilege::DELETE_ATTACHMENT,                         defaultPrivileges.forumWide.deleteAttachment);
    store.setForumWidePrivilege(ForumWidePrivilege::NO_THROTTLING,                             defaultPrivileges.forumWide.noThrottling);

    const auto& defaultPrivilegeGrants = config->defaultPrivilegeGrants;
    {
        PrivilegeDefaultLevel defaults;

        defaults.value = defaultPrivilegeGrants.thread.create.value;
        defaults.duration = defaultPrivilegeGrants.thread.create.duration;
        store.setForumWideDefaultPrivilegeLevel(ForumWideDefaultPrivilegeDuration::CREATE_DISCUSSION_THREAD, defaults);

        defaults.value = defaultPrivilegeGrants.threadMessage.create.value;
        defaults.duration = defaultPrivilegeGrants.threadMessage.create.duration;
        store.setForumWideDefaultPrivilegeLevel(ForumWideDefaultPrivilegeDuration::CREATE_DISCUSSION_THREAD_MESSAGE, defaults);
    }
}

EntityCollection::EntityCollection(const StringView messagesFile)
{
    impl_ = new Impl(messagesFile);

    Private::setGlobalEntityCollection(this);
    impl_->setEventListeners();

    anonymousUser_ = UserPtr(static_cast<UserPtr::IndexType>(impl_->managedEntities.users.add("<anonymous>")));
    anonymousUserId_ = anonymousUser_->id();

    loadDefaultPrivilegeValues(*this);
}

EntityCollection::~EntityCollection()
{
    Private::setGlobalEntityCollection(nullptr);

    delete impl_;
}

const GrantedPrivilegeStore& EntityCollection::grantedPrivileges() const
{
    return impl_->grantedPrivileges_;
}

GrantedPrivilegeStore& EntityCollection::grantedPrivileges()
{
    return impl_->grantedPrivileges_;
}

std::unique_ptr<User>* EntityCollection::getUserPoolRoot()
{
    return impl_->managedEntities.users.data();
}

std::unique_ptr<DiscussionThread>* EntityCollection::getDiscussionThreadPoolRoot()
{
    return impl_->managedEntities.threads.data();
}

std::unique_ptr<DiscussionThreadMessage>* EntityCollection::getDiscussionThreadMessagePoolRoot()
{
    return impl_->managedEntities.threadMessages.data();
}

std::unique_ptr<DiscussionTag>* EntityCollection::getDiscussionTagPoolRoot()
{
    return impl_->managedEntities.tags.data();
}

std::unique_ptr<DiscussionCategory>* EntityCollection::getDiscussionCategoryPoolRoot()
{
    return impl_->managedEntities.categories.data();
}

std::unique_ptr<MessageComment>* EntityCollection::getMessageCommentPoolRoot()
{
    return impl_->managedEntities.messageComments.data();
}

std::unique_ptr<PrivateMessage>* EntityCollection::getPrivateMessagePoolRoot()
{
    return impl_->managedEntities.privateMessages.data();
}

std::unique_ptr<Attachment>* EntityCollection::getAttachmentPoolRoot()
{
    return impl_->managedEntities.attachments.data();
}

StringView EntityCollection::getMessageContentPointer(size_t offset, size_t size)
{
    return impl_->getMessageContentPointer(offset, size);
}

UserPtr EntityCollection::createUser(IdType id, User::NameType&& name, Timestamp created, VisitDetails creationDetails)
{
    auto result = UserPtr(static_cast<UserPtr::IndexType>(
        impl_->managedEntities.users.add(id, std::move(name), created, creationDetails)));
    result->pointer_ = result;
    return result;
}

DiscussionThreadPtr EntityCollection::createDiscussionThread(IdType id, User& createdBy, DiscussionThread::NameType&& name,
                                                             Timestamp created, VisitDetails creationDetails,
                                                             const bool approved)
{
    auto result = DiscussionThreadPtr(static_cast<DiscussionThreadPtr::IndexType>(impl_->managedEntities.threads.add(
        id, createdBy, std::move(name), created, creationDetails, *this, approved)));
    result->pointer_ = result;
    return result;
}

DiscussionThreadMessagePtr EntityCollection::createDiscussionThreadMessage(IdType id, User& createdBy,
                                                                           const Timestamp created,
                                                                           const VisitDetails creationDetails,
                                                                           const bool approved)
{
    return DiscussionThreadMessagePtr(static_cast<DiscussionThreadMessagePtr::IndexType>(
        impl_->managedEntities.threadMessages.add(id, createdBy, created, creationDetails, approved)));
}

DiscussionTagPtr EntityCollection::createDiscussionTag(IdType id, DiscussionTag::NameType&& name, Timestamp created,
                                                       VisitDetails creationDetails)
{
    auto result = DiscussionTagPtr(static_cast<DiscussionTagPtr::IndexType>(impl_->managedEntities.tags.add(
        id, std::move(name), created, creationDetails, *this)));
    result->pointer_ = result;
    return result;
}

DiscussionCategoryPtr EntityCollection::createDiscussionCategory(IdType id, DiscussionCategory::NameType&& name,
                                                                 Timestamp created, VisitDetails creationDetails)
{
    auto result = DiscussionCategoryPtr(static_cast<DiscussionCategoryPtr::IndexType>(impl_->managedEntities.categories.add(
        id, std::move(name), created, creationDetails, *this)));
    result->pointer_ = result;
    return result;
}

MessageCommentPtr EntityCollection::createMessageComment(IdType id, DiscussionThreadMessage& message, User& createdBy,
                                                         Timestamp created, VisitDetails creationDetails)
{
    return MessageCommentPtr(static_cast<MessageCommentPtr::IndexType>(impl_->managedEntities.messageComments.add(
        id, message, createdBy, created, creationDetails)));
}

PrivateMessagePtr EntityCollection::createPrivateMessage(IdType id, User& source, User& destination, Timestamp created,
                                                         VisitDetails creationDetails, PrivateMessage::ContentType&& content)
{
    return PrivateMessagePtr(static_cast<PrivateMessagePtr::IndexType>(impl_->managedEntities.privateMessages.add(
        id, source, destination, created, creationDetails, std::move(content))));
}

AttachmentPtr EntityCollection::createAttachment(const IdType id, const Timestamp created, const VisitDetails creationDetails,
                                                 User& createdBy, Attachment::NameType&& name, const uint64_t size, 
                                                 const bool approved)
{
    return AttachmentPtr(static_cast<AttachmentPtr::IndexType>(impl_->managedEntities.attachments.add(
        id, created, creationDetails, createdBy, std::move(name), size, approved)));
}


const UserCollection& EntityCollection::users() const
{
    return impl_->users_;
}

UserCollection& EntityCollection::users()
{
    return impl_->users_;
}

const DiscussionThreadCollectionWithHashedId& EntityCollection::threads() const
{
    return impl_->threads_;
}

DiscussionThreadCollectionWithHashedId& EntityCollection::threads()
{
    return impl_->threads_;
}

const DiscussionThreadMessageCollection& EntityCollection::threadMessages() const
{
    return impl_->threadMessages_;
}

DiscussionThreadMessageCollection& EntityCollection::threadMessages()
{
    return impl_->threadMessages_;
}

const DiscussionTagCollection& EntityCollection::tags() const
{
    return impl_->tags_;
}

DiscussionTagCollection& EntityCollection::tags()
{
    return impl_->tags_;
}

const DiscussionCategoryCollection& EntityCollection::categories() const
{
    return impl_->categories_;
}

DiscussionCategoryCollection& EntityCollection::categories()
{
    return impl_->categories_;
}

const MessageCommentCollection& EntityCollection::messageComments() const
{
    return impl_->messageComments_;
}

MessageCommentCollection& EntityCollection::messageComments()
{
    return impl_->messageComments_;
}

const PrivateMessageGlobalCollection& EntityCollection::privateMessages() const
{
    return impl_->privateMessages_;
}

PrivateMessageGlobalCollection& EntityCollection::privateMessages()
{
    return impl_->privateMessages_;
}

const AttachmentCollection& EntityCollection::attachments() const
{
    return impl_->attachments_;
}

AttachmentCollection& EntityCollection::attachments()
{
    return impl_->attachments_;
}

void EntityCollection::insertUser(UserPtr user)
{
    impl_->insertUser(user);
}

void EntityCollection::deleteUser(UserPtr user)
{
    impl_->deleteUser(user);
}

void EntityCollection::insertDiscussionThread(DiscussionThreadPtr thread)
{
    impl_->insertDiscussionThread(thread);
}

void EntityCollection::deleteDiscussionThread(DiscussionThreadPtr thread, const bool deleteMessages)
{
    impl_->deleteDiscussionThread(thread, deleteMessages);
}

void EntityCollection::insertDiscussionThreadMessage(DiscussionThreadMessagePtr message)
{
    impl_->insertDiscussionThreadMessage(message);
}

void EntityCollection::deleteDiscussionThreadMessage(DiscussionThreadMessagePtr message)
{
    impl_->deleteDiscussionThreadMessage(message);
}

void EntityCollection::insertDiscussionTag(DiscussionTagPtr tag)
{
    impl_->insertDiscussionTag(tag);
}

void EntityCollection::deleteDiscussionTag(DiscussionTagPtr tag)
{
    impl_->deleteDiscussionTag(tag);
}

void EntityCollection::insertDiscussionCategory(DiscussionCategoryPtr category)
{
    impl_->insertDiscussionCategory(category);
}

void EntityCollection::deleteDiscussionCategory(DiscussionCategoryPtr category)
{
    impl_->deleteDiscussionCategory(category);
}

void EntityCollection::insertMessageComment(MessageCommentPtr comment)
{
    impl_->insertMessageComment(comment);
}

void EntityCollection::deleteMessageComment(MessageCommentPtr comment)
{
    impl_->deleteMessageComment(comment);
}

void EntityCollection::insertPrivateMessage(PrivateMessagePtr message)
{
    impl_->insertPrivateMessage(message);
}

void EntityCollection::deletePrivateMessage(PrivateMessagePtr message)
{
    impl_->deletePrivateMessage(message);
}

void EntityCollection::insertAttachment(AttachmentPtr attachment)
{
    impl_->insertAttachment(attachment);
}

void EntityCollection::deleteAttachment(AttachmentPtr attachment)
{
    impl_->deleteAttachment(attachment);
}

void EntityCollection::startBatchInsert()
{
    impl_->toggleBatchInsert(true);
}

void EntityCollection::stopBatchInsert()
{
    impl_->toggleBatchInsert(false);
}
