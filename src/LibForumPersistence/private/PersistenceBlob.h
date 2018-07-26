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

#pragma once

#include <cstddef>

namespace Forum::Persistence
{
    struct Blob final
    {
        char* buffer{nullptr}; //storing raw pointer so that Blob can be placed in a boost lockfree queue
        size_t size{};

        //Blob& operator=(const Blob&) = default;

        static Blob withSize(const size_t size)
        {
            return { new char[size], size };
        }

        static void free(Blob& blob)
        {
            delete[] blob.buffer;
            blob.buffer = nullptr;
        }
    };
}
