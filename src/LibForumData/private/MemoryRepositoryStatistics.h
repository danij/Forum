#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryStatistics final : public MemoryRepositoryBase, public IStatisticsRepository
        {
        public:
            explicit MemoryRepositoryStatistics(MemoryStoreRef store,
                                                Authorization::StatisticsAuthorizationRef authorization);

            StatusCode getEntitiesCount(OutStream& output) const override;

        private:
            Authorization::StatisticsAuthorizationRef authorization_;
        };
    }
}
