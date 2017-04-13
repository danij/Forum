#include "ServiceEndpointManager.h"
#include "ServiceEndpoints.h"

using namespace Forum;
using namespace Forum::Commands;

struct ServiceEndpointManager::ServiceEndpointManagerImpl
{
    explicit ServiceEndpointManagerImpl(CommandHandler& handler)
        : commandHandler(handler), metricsEndpoint(handler), statisticsEndpoint(handler)
    {
    }

    CommandHandler& commandHandler;
    MetricsEndpoint metricsEndpoint;
    StatisticsEndpoint statisticsEndpoint;
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
}
