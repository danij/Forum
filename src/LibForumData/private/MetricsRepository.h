#pragma once

#include "Repository.h"
#include "Authorization.h"
#include "MemoryRepositoryCommon.h"

namespace Forum
{
    namespace Repository
    {
        class MetricsRepository : public MemoryRepositoryBase, public IMetricsRepository
        {
        public:
            MetricsRepository(MemoryStoreRef store, Authorization::MetricsAuthorizationRef authorization);

            StatusCode getVersion(OutStream& output) override;

        private:
            Authorization::MetricsAuthorizationRef authorization_;
        };
    }
}
