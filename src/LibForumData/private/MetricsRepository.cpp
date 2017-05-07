#include "MetricsRepository.h"
#include "MemoryRepositoryCommon.h"

#include "OutputHelpers.h"
#include "Version.h"

using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MetricsRepository::MetricsRepository(MemoryStoreRef store, MetricsAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MetricsRepository::getVersion(OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const Entities::EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
                      
                          authorizationStatus = authorization_->getVersion(currentUser);
                          if (AuthorizationStatus::OK != authorizationStatus)
                          {
                              status = authorizationStatus;
                              return;
                          }
                      
                          status.disable();
                          writeSingleValueSafeName(output, "version", VERSION);
                      });
    return status;
}
