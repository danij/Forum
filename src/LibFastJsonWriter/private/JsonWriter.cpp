/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "JsonWriter.h"
#include <algorithm>

using namespace Json;

bool Json::isEscapeNeeded(const char* value, const size_t length)
{
    return std::any_of(value, value + length, [](const char c)
    {
        const auto u = static_cast<unsigned char>(c);
        return (u < ToEscapeLength) && ToEscape[u];
    });
}
