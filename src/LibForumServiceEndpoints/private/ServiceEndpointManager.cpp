#include "ServiceEndpointManager.h"
#include "ServiceEndpoints.h"

using namespace Forum;
using namespace Forum::Commands;

struct ServiceEndpointManager::ServiceEndpointManagerImpl
{
    explicit ServiceEndpointManagerImpl(CommandHandler& handler)
        : commandHandler(handler), metricsEndpoint(handler), statisticsEndpoint(handler), 
          usersEndpoint(handler), threadsEndpoint(handler), threadMessagesEndpoint(handler),
          tagsEndpoint(handler), categoriesEndpoint(handler)
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
    router.addRoute("metrics/version/", Http::HttpVerb::GET, 
                    [this](auto& state) { this->impl_->metricsEndpoint.getVersion(state);});
    router.addRoute("statistics/entitycount/", Http::HttpVerb::GET, 
                    [this](auto& state) { this->impl_->statisticsEndpoint.getEntitiesCount(state);});
    
    router.addRoute("users/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getAll(state);});
    router.addRoute("users/id", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getUserById(state);});
    router.addRoute("users/name/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getUserByName(state);});
    router.addRoute("users/", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->usersEndpoint.add(state);});
    router.addRoute("users/", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->usersEndpoint.remove(state);});
    router.addRoute("users/name/", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->usersEndpoint.changeName(state);});
    router.addRoute("users/info/", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->usersEndpoint.changeInfo(state);});

    router.addRoute("threads/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getAll(state);});
    router.addRoute("threads/id", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadById(state);});
    router.addRoute("threads/user/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsOfUser(state);});
    router.addRoute("threads/subscribed/user/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getSubscribedThreadsOfUser(state);});
    router.addRoute("threads/tag/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsWithTag(state);});
    router.addRoute("threads/category/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsOfCategory(state);});
    router.addRoute("threads/", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadsEndpoint.add(state);});
    router.addRoute("threads/", Http::HttpVerb::DELETE,
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

    router.addRoute("messages/user/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getThreadMessagesOfUser(state);});
    router.addRoute("messages/allcomments/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getAllComments(state);});
    router.addRoute("messages/comments/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getCommentsOfMessage(state);});
    router.addRoute("messages/commentsofuser/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getCommentsOfUser(state);});
    router.addRoute("messages/", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.add(state);});
    router.addRoute("messages/", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.remove(state);});
    router.addRoute("messages/content", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.changeContent(state);});
    router.addRoute("messages/move", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.move(state);});
    router.addRoute("messages/upvote", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.upVote(state);});
    router.addRoute("messages/downvote", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.downVote(state);});
    router.addRoute("messages/resetvote", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.resetVote(state);});
    router.addRoute("messages/comment", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.addComment(state);});
    router.addRoute("messages/comment/solved", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->threadMessagesEndpoint.setCommentSolved(state);});

    router.addRoute("tags/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->tagsEndpoint.getAll(state);});
    router.addRoute("tags/", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->tagsEndpoint.add(state);});
    router.addRoute("tags/", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->tagsEndpoint.remove(state);});
    router.addRoute("tags/name", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->tagsEndpoint.changeName(state);});
    router.addRoute("tags/uiblob", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->tagsEndpoint.changeUiBlob(state);});
    router.addRoute("tags/merge", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->tagsEndpoint.merge(state);});

    router.addRoute("categories/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getAll(state);});
    router.addRoute("categories/root/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getRootCategories(state);});
    router.addRoute("category/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getCategoryById(state);});
    router.addRoute("categories/", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->categoriesEndpoint.add(state);});
    router.addRoute("categories/", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->categoriesEndpoint.remove(state);});
    router.addRoute("categories/name", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeName(state);});
    router.addRoute("categories/description", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeDescription(state);});
    router.addRoute("categories/parent", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeParent(state);});
    router.addRoute("categories/displayOrder", Http::HttpVerb::PUT,
                    [this](auto& state) { this->impl_->categoriesEndpoint.changeDisplayOrder(state);});
    router.addRoute("categories/tag", Http::HttpVerb::POST,
                    [this](auto& state) { this->impl_->categoriesEndpoint.addTag(state);});
    router.addRoute("categories/tag", Http::HttpVerb::DELETE,
                    [this](auto& state) { this->impl_->categoriesEndpoint.removeTag(state);});

}
