#include "ServiceEndpointManager.h"
#include "ServiceEndpoints.h"

#include <tuple>

using namespace Forum;
using namespace Forum::Commands;
using namespace Http;

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

#define ENDPOINT_DELEGATE(method) [this](auto& state) { this->impl_->method(state); }

void ServiceEndpointManager::registerRoutes(HttpRouter& router)
{
    std::tuple<StringView, HttpVerb, HttpRouter::HandlerFn> routes[] =
    {
        { "metrics/version",        HttpVerb::GET, ENDPOINT_DELEGATE(metricsEndpoint.getVersion) },
        { "statistics/entitycount", HttpVerb::GET, ENDPOINT_DELEGATE(statisticsEndpoint.getEntitiesCount) },

        { "users",             HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getAll) },
        { "users/online",      HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getOnline) },
        { "users/id",          HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserById) },
        { "users/name",        HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserByName) },
        { "users",             HttpVerb::POST,   ENDPOINT_DELEGATE(usersEndpoint.add) },
        { "users",             HttpVerb::DELETE, ENDPOINT_DELEGATE(usersEndpoint.remove) },
        { "users/name",        HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeName) },
        { "users/info",        HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeInfo) },
        { "users/title",       HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeTitle) },
        { "users/signature",   HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeSignature) },
        { "users/logo",        HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserLogo) },
        { "users/logo",        HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeLogo) },
        { "users/logo",        HttpVerb::DELETE, ENDPOINT_DELEGATE(usersEndpoint.deleteLogo) },
        { "users/votehistory", HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserVoteHistory) },

        { "threads",                 HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getAll) },
        { "threads/id",              HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadById) },
        { "threads/user",            HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadsOfUser) },
        { "threads/subscribed/user", HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getSubscribedThreadsOfUser) },
        { "threads/tag",             HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadsWithTag) },
        { "threads/category",        HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadsOfCategory) },
        { "threads",                 HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.add) },
        { "threads",                 HttpVerb::DELETE, ENDPOINT_DELEGATE(threadsEndpoint.remove) },
        { "threads/name",            HttpVerb::PUT,    ENDPOINT_DELEGATE(threadsEndpoint.changeName) },
        { "threads/pindisplayorder", HttpVerb::PUT,    ENDPOINT_DELEGATE(threadsEndpoint.changePinDisplayOrder) },
        { "threads/merge",           HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.merge) },
        { "threads/subscribe",       HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.subscribe) },
        { "threads/unsubscribe",     HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.unsubscribe) },
        { "threads/tag",             HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.addTag) },
        { "threads/tag",             HttpVerb::DELETE, ENDPOINT_DELEGATE(threadsEndpoint.removeTag) },

        { "thread_messages/user",           HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getThreadMessagesOfUser) },
        { "thread_messages/allcomments",    HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getAllComments) },
        { "thread_messages/comments",       HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getCommentsOfMessage) },
        { "thread_messages/comments/user",  HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getCommentsOfUser) },
        { "thread_messages/rank",           HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getRankOfMessage) },
        { "thread_messages",                HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.add) },
        { "thread_messages",                HttpVerb::DELETE, ENDPOINT_DELEGATE(threadMessagesEndpoint.remove) },
        { "thread_messages/content",        HttpVerb::PUT,    ENDPOINT_DELEGATE(threadMessagesEndpoint.changeContent) },
        { "thread_messages/move",           HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.move) },
        { "thread_messages/upvote",         HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.upVote) },
        { "thread_messages/downvote",       HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.downVote) },
        { "thread_messages/resetvote",      HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.resetVote) },
        { "thread_messages/comment",        HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.addComment) },
        { "thread_messages/comment/solved", HttpVerb::PUT,    ENDPOINT_DELEGATE(threadMessagesEndpoint.setCommentSolved) },

        { "tags",        HttpVerb::GET,    ENDPOINT_DELEGATE(tagsEndpoint.getAll) },
        { "tags",        HttpVerb::POST,   ENDPOINT_DELEGATE(tagsEndpoint.add) },
        { "tags",        HttpVerb::DELETE, ENDPOINT_DELEGATE(tagsEndpoint.remove) },
        { "tags/name",   HttpVerb::PUT,    ENDPOINT_DELEGATE(tagsEndpoint.changeName) },
        { "tags/uiblob", HttpVerb::PUT,    ENDPOINT_DELEGATE(tagsEndpoint.changeUiBlob) },
        { "tags/merge",  HttpVerb::POST,   ENDPOINT_DELEGATE(tagsEndpoint.merge) },

        { "categories",              HttpVerb::GET,    ENDPOINT_DELEGATE(categoriesEndpoint.getAll) },
        { "categories/root",         HttpVerb::GET,    ENDPOINT_DELEGATE(categoriesEndpoint.getRootCategories) },
        { "category",                HttpVerb::GET,    ENDPOINT_DELEGATE(categoriesEndpoint.getCategoryById) },
        { "categories",              HttpVerb::POST,   ENDPOINT_DELEGATE(categoriesEndpoint.add) },
        { "categories",              HttpVerb::DELETE, ENDPOINT_DELEGATE(categoriesEndpoint.remove) },
        { "categories/name",         HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeName) },
        { "categories/description",  HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeDescription) },
        { "categories/parent",       HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeParent) },
        { "categories/displayorder", HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeDisplayOrder) },
        { "categories/tag",          HttpVerb::POST,   ENDPOINT_DELEGATE(categoriesEndpoint.addTag) },
        { "categories/tag",          HttpVerb::DELETE, ENDPOINT_DELEGATE(categoriesEndpoint.removeTag) },

        { "privileges/required/thread_message",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForThreadMessage) },
        { "privileges/assigned/thread_message",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForThreadMessage) },
        { "privileges/required/thread",          HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForThread) },
        { "privileges/assigned/thread_message",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForThread) },
        { "privileges/required/tag",             HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForTag) },
        { "privileges/assigned/tag",             HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForTag) },
        { "privileges/required/category",        HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForCategory) },
        { "privileges/assigned/thread_message",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForCategory) },
        { "privileges/forum_wide/current_user",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideCurrentUserPrivileges) },
        { "privileges/required/forum_wide",      HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideRequiredPrivileges) },
        { "privileges/defaults/forum_wide",      HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideDefaultPrivilegeLevels) },
        { "privileges/assigned/forum_wide",      HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideAssignedPrivileges) },
        { "privileges/assigned/forum_wide/user", HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideAssignedPrivilegesForUser) },

        { "privileges/thread_message/required/thread_message", HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage) },
        { "privileges/thread_message/required/thread",         HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForThread) },
        { "privileges/thread/required/thread",                 HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadRequiredPrivilegeForThread) },
        { "privileges/thread_message/required/tag",            HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForTag) },
        { "privileges/thread/required/tag",                    HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadRequiredPrivilegeForTag) },
        { "privileges/tag/required/tag",                       HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionTagRequiredPrivilegeForTag) },
        { "privileges/category/required/category",             HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionCategoryRequiredPrivilegeForCategory) },
        { "privileges/thread_message/required/forum_wide",     HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilege) },
        { "privileges/thread/required/forum_wide",             HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadRequiredPrivilege) },
        { "privileges/tag/required/forum_wide",                HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionTagRequiredPrivilege) },
        { "privileges/category/required/forum_wide",           HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionCategoryRequiredPrivilege) },
        { "privileges/forum_wide/required/forum_wide",         HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeForumWideRequiredPrivilege) },
        { "privileges/forum_wide/defaults/forum_wide",         HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeForumWideDefaultPrivilegeLevel) },

        { "privileges/thread_message/assign",                  HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionThreadMessagePrivilege) },
        { "privileges/thread/assign",                          HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionThreadPrivilege) },
        { "privileges/tag/assign",                             HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionTagPrivilege) },
        { "privileges/category/assign",                        HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionCategoryPrivilege) },
        { "privileges/forum_wide/assign",                      HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignForumWidePrivilege) }
    };

    for (auto& tuple : routes)
    {
        router.addRoute(std::get<0>(tuple), std::get<1>(tuple), std::move(std::get<2>(tuple)));
    }
}
