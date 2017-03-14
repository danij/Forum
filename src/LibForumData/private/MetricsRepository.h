#pragma once

#include "Repository.h"

#include <boost/core/noncopyable.hpp>

namespace Forum
{
    namespace Repository
    {
        class MetricsRepository : public IMetricsRepository, private boost::noncopyable
        {
            StatusCode getVersion(OutStream& output) override;

        };
    }
}