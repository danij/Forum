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

        template<typename T>
        void write(T smallValue)
        {
            static_assert(std::is_trivial<T>::value, "T must be primitive");
            static_assert(std::is_integral<T>::value, "T must be integral");
            static_assert( ! std::is_pointer<T>::value, "T must not be a pointer");

            if (capacity_ < (used_ + sizeof(T)))
            {
                resize();
            }
            *(reinterpret_cast<T*>(buffer_.get() + used_)) = smallValue;
            used_ += sizeof(T);
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
