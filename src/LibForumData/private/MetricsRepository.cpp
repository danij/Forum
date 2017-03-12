#include "MetricsRepository.h"

#include "OutputHelpers.h"
#include "Version.h"

using namespace Forum::Helpers;
using namespace Forum::Repository;

StatusCode MetricsRepository::getVersion(std::ostream& output)
{
    writeSingleValueSafeName(output, "version", VERSION);
    return StatusCode::OK;
}
