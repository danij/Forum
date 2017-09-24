#include "ServiceEndpointManager.h"
#include "ServiceEndpoints.h"

using namespace Forum;
using namespace Forum::Commands;

struct ServiceEndpointManager::ServiceEndpointManagerImpl
{
    explicit ServiceEndpointManagerImpl(CommandHandler& handler)
        : commandHandler(handler), metricsEndpoint(handler), statisticsEndpoint(handler),
          usersEndpoint(handler), threadsEndpoint(handler), threadMessagesEndpoint(handler),
          tagsEndpoint(handler), categoriesEndpoint(handler), authorizationEndpoint(handler)
    {
    }

    CommandHandler& commandHandler;
    MetricsEndpoint metricsEndpoint;
    StatisticsEndpoint statisticsEndpoint;
    UsersEndpoint usersEndpoint;
    DiscussionThreadsEndpoint threadsEndpoint;
    DiscussionThreadMessagesEndpoint threadMessagesEndpoint;
    DiscussionTagsEndpoint tagsEndpoint;
    DiscussionCategoriesEndpoint categoriesEndpoint;
    AuthorizationEndpoint authorizationEndpoint;
};

ServiceEndpointManager::ServiceEndpointManager(CommandHandler& handler)
{
    impl_ = new ServiceEndpointManagerImpl(handler);
}

ServiceEndpointManager::~ServiceEndpointManager()
{
    if (impl_)
    {
        delete impl_;
    }
}

void ServiceEndpointManager::registerRoutes(Http::HttpRouter& router)
{
    router.addRoute("metrics/version", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->metricsEndpoint.getVersion(state);});
    router.addRoute("statistics/entitycount", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->statisticsEndpoint.getEntitiesCount(state);});

    router.addRoute("users", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getAll(state);});
    router.addRoute("users/id", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getUserById(state);});
    router.addRoute("users/name", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getUserByName(state);});
    router.addRoute("users", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->usersEndpoint.add(state);});
    router.addRoute("users", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->usersEndpoint.remove(state);});
    router.addRoute("users/name", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->usersEndpoint.changeName(state);});
    router.addRoute("users/info", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->usersEndpoint.changeInfo(state);});

    router.addRoute("threads", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getAll(state);});
    router.addRoute("threads/id", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadById(state);});
    router.addRoute("threads/user", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsOfUser(state);});
    router.addRoute("threads/subscribed/user", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getSubscribedThreadsOfUser(state);});
    router.addRoute("threads/tag", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsWithTag(state);});
    router.addRoute("threads/category", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsOfCategory(state);});
    router.addRoute("threads", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadsEndpoint.add(state);});
    router.addRoute("threads", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->threadsEndpoint.remove(state);});
    router.addRoute("threads/name", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->threadsEndpoint.changeName(state);});
    router.addRoute("threads/pindisplayorder", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->threadsEndpoint.changePinDisplayOrder(state);});
    router.addRoute("threads/merge", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadsEndpoint.merge(state);});
    router.addRoute("threads/subscribe", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadsEndpoint.subscribe(state);});
    router.addRoute("threads/unsubscribe", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadsEndpoint.unsubscribe(state);});
    router.addRoute("threads/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadsEndpoint.addTag(state);});
    router.addRoute("threads/tag", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->threadsEndpoint.removeTag(state);});

    router.addRoute("thread_messages/user", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getThreadMessagesOfUser(state);});
    router.addRoute("thread_messages/allcomments", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getAllComments(state);});
    router.addRoute("thread_messages/comments", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getCommentsOfMessage(state);});
    router.addRoute("thread_messages/comments/user", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getCommentsOfUser(state);});
    router.addRoute("thread_messages", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.add(state);});
    router.addRoute("thread_messages", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.remove(state);});
    router.addRoute("thread_messages/content", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.changeContent(state);});
    router.addRoute("thread_messages/move", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.move(state);});
    router.addRoute("thread_messages/upvote", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.upVote(state);});
    router.addRoute("thread_messages/downvote", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.downVote(state);});
    router.addRoute("thread_messages/resetvote", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.resetVote(state);});
    router.addRoute("thread_messages/comment", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.addComment(state);});
    router.addRoute("thread_messages/comment/solved", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.setCommentSolved(state);});

    router.addRoute("tags", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->tagsEndpoint.getAll(state);});
    router.addRoute("tags", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->tagsEndpoint.add(state);});
    router.addRoute("tags", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->tagsEndpoint.remove(state);});
    router.addRoute("tags/name", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->tagsEndpoint.changeName(state);});
    router.addRoute("tags/uiblob", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->tagsEndpoint.changeUiBlob(state);});
    router.addRoute("tags/merge", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->tagsEndpoint.merge(state);});

    router.addRoute("categories", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getAll(state);});
    router.addRoute("categories/root", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getRootCategories(state);});
    router.addRoute("category", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getCategoryById(state);});
    router.addRoute("categories", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->categoriesEndpoint.add(state);});
    router.addRoute("categories", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->categoriesEndpoint.remove(state);});
    router.addRoute("categories/name", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeName(state);});
    router.addRoute("categories/description", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeDescription(state);});
    router.addRoute("categories/parent", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeParent(state);});
    router.addRoute("categories/displayorder", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeDisplayOrder(state);});
    router.addRoute("categories/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->categoriesEndpoint.addTag(state);});
    router.addRoute("categories/tag", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->categoriesEndpoint.removeTag(state);});

    router.addRoute("privileges/required/thread_message", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getRequiredPrivilegesForThreadMessage(state);});
    router.addRoute("privileges/assigned/thread_message", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getAssignedPrivilegesForThreadMessage(state);});
    router.addRoute("privileges/required/thread", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getRequiredPrivilegesForThread(state);});
    router.addRoute("privileges/durations/thread", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getDefaultPrivilegeDurationsForThread(state);});
    router.addRoute("privileges/assigned/thread_message", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getAssignedPrivilegesForThread(state);});
    router.addRoute("privileges/required/tag", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getRequiredPrivilegesForTag(state);});
    router.addRoute("privileges/durations/tag", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getDefaultPrivilegeDurationsForTag(state);});
    router.addRoute("privileges/assigned/tag", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getAssignedPrivilegesForTag(state);});
    router.addRoute("privileges/required/category", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getRequiredPrivilegesForCategory(state);});
    router.addRoute("privileges/assigned/thread_message", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getAssignedPrivilegesForCategory(state);});
    router.addRoute("privileges/forum_wide/current_user", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getForumWideCurrentUserPrivileges(state);});
    router.addRoute("privileges/required/forum_wide", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getForumWideRequiredPrivileges(state);});
    router.addRoute("privileges/durations/forum_wide", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getForumWideDefaultPrivilegeDurations(state);});
    router.addRoute("privileges/assinged/forum_wide", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getForumWideAssignedPrivileges(state);});
    router.addRoute("privileges/assinged/forum_wide/user", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->authorizationEndpoint.getForumWideAssignedPrivilegesForUser(state);});

    router.addRoute("privileges/thread_message/required/thread_message", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(state);});
    router.addRoute("privileges/thread_message/assign/thread_message", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadMessagePrivilegeForThreadMessage(state);});
    router.addRoute("privileges/thread_message/required/thread", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForThread(state);});
    router.addRoute("privileges/thread/required/thread", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadRequiredPrivilegeForThread(state);});
    router.addRoute("privileges/thread_message/duration/thread", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(state);});
    router.addRoute("privileges/thread_message/assign/thread", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadMessagePrivilegeForThread(state);});
    router.addRoute("privileges/thread/assign/thread", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadPrivilegeForThread(state);});
    router.addRoute("privileges/thread_message/required/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForTag(state);});
    router.addRoute("privileges/thread/required/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadRequiredPrivilegeForTag(state);});
    router.addRoute("privileges/tag/required/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionTagRequiredPrivilegeForTag(state);});
    router.addRoute("privileges/thread_message/duration/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(state);});
    router.addRoute("privileges/thread_message/assign/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadMessagePrivilegeForTag(state);});
    router.addRoute("privileges/thread/assign/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadPrivilegeForTag(state);});
    router.addRoute("privileges/tag/required/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionTagPrivilegeForTag(state);});
    router.addRoute("privileges/category/required/category", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionCategoryRequiredPrivilegeForCategory(state);});
    router.addRoute("privileges/category/assign/category", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionCategoryPrivilegeForCategory(state);});
    router.addRoute("privileges/thread_message/required/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilege(state);});
    router.addRoute("privileges/thread/required/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadRequiredPrivilege(state);});
    router.addRoute("privileges/tag/required/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionTagRequiredPrivilege(state);});
    router.addRoute("privileges/category/required/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionCategoryRequiredPrivilege(state);});
    router.addRoute("privileges/forum_wide/required/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeForumWideRequiredPrivilege(state);});
    router.addRoute("privileges/thread_message/duration/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeDiscussionThreadMessageDefaultPrivilegeDuration(state);});
    router.addRoute("privileges/forum_wide/duration/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.changeForumWideDefaultPrivilegeDuration(state);});
    router.addRoute("privileges/thread_message/assign/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadMessagePrivilege(state);});
    router.addRoute("privileges/thread/assign/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionThreadPrivilege(state);});
    router.addRoute("privileges/tag/assign/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionTagPrivilege(state);});
    router.addRoute("privileges/category/assign/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignDiscussionCategoryPrivilege(state);});
    router.addRoute("privileges/forum_wide/assign/forum_wide", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->authorizationEndpoint.assignForumWidePrivilege(state);});

}
