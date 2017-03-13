#pragma once

#include "MemoryRepositoryCommon.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryStatistics final : public MemoryRepositoryBase, public IStatisticsRepository
        {
        public:
            explicit MemoryRepositoryStatistics(MemoryStoreRef store);

            StatusCode getEntitiesCount(OutStream& output) const override;
        };
    }
}
