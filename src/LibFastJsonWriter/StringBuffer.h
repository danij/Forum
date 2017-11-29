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

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <type_traits>

#include <boost/noncopyable.hpp>
#include <boost/utility/string_view.hpp>

namespace Json
{
    class StringBuffer final : private boost::noncopyable
    {
    public:
        explicit StringBuffer(size_t growWith = 1024)
            : buffer_(new char[growWith]), growWith_(growWith), capacity_(growWith), used_(0)
        {
            assert(growWith >= 128);
        }

        StringBuffer(StringBuffer&&) = default;
        StringBuffer& operator=(StringBuffer&&) = default;

        void clear()
        {
            used_ = 0;
        }

        void write(char value)
        {
            if (capacity_ < (used_ + sizeof(char)))
            {
                resize();
            }
            *(buffer_.get() + used_) = value;
            used_ += sizeof(char);
        }

        template<size_t Size>
        void writeFixed(const char* value)
        {
            while (capacity_ < (used_ + Size))
            {
                resize();
            }
            memcpy(buffer_.get() + used_, value, Size);
            used_ += Size;
        }

        void write(const char* value, size_t size)
        {
            while (capacity_ < (used_ + size))
            {
                resize();
            }
            std::copy(value, value + size, buffer_.get() + used_);
            used_ += size;
        }

        boost::string_view view() const
        {
            return boost::string_view(buffer_.get(), used_);
        }

    private:
        void resize()
        {
            //on allocation exception, don't corrupt the state of the object
            auto newBuffer = std::unique_ptr<char[]>(new char[capacity_ + growWith_]);
            capacity_ += growWith_;
            std::memcpy(newBuffer.get(), buffer_.get(), used_);
            std::swap(buffer_, newBuffer);
        }

        std::unique_ptr<char[]> buffer_;
        size_t growWith_;
        size_t capacity_;
        size_t used_;
    };
}
