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
    router.addRoute("user/id/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getUserById(state);});
    router.addRoute("user/name/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->usersEndpoint.getUserByName(state);});

    router.addRoute("threads/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getAll(state);});
    router.addRoute("thread/id/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadById(state);});
    router.addRoute("threads/user/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsOfUser(state);});
    router.addRoute("threads/subscribed/user/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getSubscribedThreadsOfUser(state);});
    router.addRoute("threads/tag/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsWithTag(state);});
    router.addRoute("threads/category/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->threadsEndpoint.getThreadsOfCategory(state);});

    router.addRoute("messages/user/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getThreadMessagesOfUser(state);});
    router.addRoute("messages/allcomments/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getAllComments(state);});
    router.addRoute("messages/comments/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getCommentsOfMessage(state);});
    router.addRoute("messages/commentsofuser/", Http::HttpVerb::GET,
                     [this](auto& state) { this->impl_->threadMessagesEndpoint.getCommentsOfUser(state);});

    router.addRoute("tags/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->tagsEndpoint.getAll(state);});

    router.addRoute("categories/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getAll(state);});
    router.addRoute("categories/root/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getRootCategories(state);});
    router.addRoute("category/", Http::HttpVerb::GET,
                    [this](auto& state) { this->impl_->categoriesEndpoint.getCategoryById(state);});
}
