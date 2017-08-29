#pragma once

#include <algorithm>
#include <cstddef>

namespace Json
{
    template<size_t StackSize, typename SizeType>
    class StringContainer
    {
    public:
        StringContainer();
        explicit StringContainer(size_t size);
        ~StringContainer();

        StringContainer(const StringContainer& other);
        StringContainer(StringContainer&& other) noexcept;

        StringContainer& operator=(const StringContainer& other);
        StringContainer& operator=(StringContainer&& other) noexcept;

        void resize(size_t newSize);

        char* operator*() noexcept;
        const char* operator*() const noexcept;

        SizeType& size() noexcept;
        SizeType size() const noexcept;

        friend void swap(StringContainer& first, StringContainer& second) noexcept
        {
            if (StackSize >= sizeof(first.buffer_.external))
            {
                for (size_t i = 0; i < StackSize; ++i)
                {
                    std::swap(first.buffer_.stack[i], second.buffer_.stack[i]);
                }
            }
            else
            {
                std::swap(first.buffer_.external, second.buffer_.external);
            }
            std::swap(first.size_, second.size_);
        }

    private:
        bool usingHeapBuffer() const noexcept;

        union
        {
            char stack[StackSize];
            char* external = nullptr;
        } buffer_;

        SizeType size_;
    };

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>::StringContainer() : size_({})
    {
    }

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>::StringContainer(size_t size) : size_(size)
    {
        if (usingHeapBuffer())
        {
            buffer_.external = new char[size];
        }
    }

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>::~StringContainer()
    {
        if (usingHeapBuffer() && buffer_.external)
        {
            delete[] buffer_.external;
            buffer_.external = nullptr;
        }
    }

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>::StringContainer(const StringContainer& other)
        : size_(other.size_)
    {
        auto source = other.buffer_.stack;
        auto destination = buffer_.stack;
        auto size = static_cast<size_t>(size_);

        if (usingHeapBuffer())
        {
            destination = buffer_.external = new char[size];
            source = other.buffer_.external;
        }

        std::copy(source, source + size, destination);
    }

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>::StringContainer(StringContainer&& other) noexcept
    {
        swap(*this, other);
    }

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>& StringContainer<StackSize, SizeType>::operator=(const StringContainer& other)
    {
        StringContainer<StackSize, SizeType> copy(other);
        swap(*this, copy);
        return *this;
    }

    template <size_t StackSize, typename SizeType>
    StringContainer<StackSize, SizeType>& StringContainer<StackSize, SizeType>::operator=(StringContainer&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    template <size_t StackSize, typename SizeType>
    void StringContainer<StackSize, SizeType>::resize(size_t newSize)
    {
        StringContainer<StackSize, SizeType> other(newSize);
        swap(*this, other);
    }

    template <size_t StackSize, typename SizeType>
    char* StringContainer<StackSize, SizeType>::operator*() noexcept
    {
        return usingHeapBuffer() ? buffer_.external : buffer_.stack;
    }

    template <size_t StackSize, typename SizeType>
    const char* StringContainer<StackSize, SizeType>::operator*() const noexcept
    {
        return usingHeapBuffer() ? buffer_.external : buffer_.stack;
    }

    template <size_t StackSize, typename SizeType>
    SizeType& StringContainer<StackSize, SizeType>::size() noexcept
    {
        return size_;
    }

    template <size_t StackSize, typename SizeType>
    SizeType StringContainer<StackSize, SizeType>::size() const noexcept
    {
        return size_;
    }

    template <size_t StackSize, typename SizeType>
    bool StringContainer<StackSize, SizeType>::usingHeapBuffer() const noexcept
    {
        return static_cast<size_t>(size_) > StackSize;
    }
}
