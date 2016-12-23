#pragma once

#include "Repository.h"

#include <boost/core/noncopyable.hpp>

namespace Forum
{
    namespace Repository
    {
        class MetricsRepository : public IMetricsRepository, private boost::noncopyable
        {
            virtual void getVersion(std::ostream& output) override;

        };
    }
}