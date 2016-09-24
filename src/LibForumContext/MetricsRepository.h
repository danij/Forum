#pragma once

#include <boost/core/noncopyable.hpp>

#include "Repository.h"

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