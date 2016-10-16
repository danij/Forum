#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "TestHelpers.h"

const std::string Forum::Helpers::emptyIdString = boost::uuids::to_string(boost::uuids::uuid());
