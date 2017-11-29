/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
const Forum::Entities::IdType Forum::Helpers::sampleValidId = Forum::Helpers::sampleValidIdString;

const std::string Forum::Helpers::sampleValidIdString2 = "00000000-0000-0000-0000-000000000002";
const Forum::Entities::IdType Forum::Helpers::sampleValidId2 = Forum::Helpers::sampleValidIdString2;

const std::string Forum::Helpers::sampleMessageContent = "abcdefghijklmnopqrstuvwxyz";
