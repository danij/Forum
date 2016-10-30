#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "TestHelpers.h"

bool Forum::Helpers::treeContains(const boost::property_tree::ptree& tree, const std::string& key)
{
    for (auto& pair : tree)
    {
        if (pair.first == key)
        {
            return true;
        }
    }
    return false;
}

const std::string Forum::Helpers::sampleValidIdString = "00000000-0000-0000-0000-000000000001";
const Forum::Entities::IdType Forum::Helpers::sampleValidId = sampleValidIdString;

const std::string Forum::Helpers::sampleMessageContent = "abcdefghijklmnopqrstuvwxyz";
