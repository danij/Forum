#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <boost/utility/string_view.hpp>

namespace Json
{
    template<size_t StackSize>
    class JsonReadyString
    {
    public:
        explicit JsonReadyString(boost::string_view source);
        ~JsonReadyString();

        JsonReadyString(const JsonReadyString& other);
        JsonReadyString(JsonReadyString&& other) noexcept;

        JsonReadyString& operator=(JsonReadyString other);
        JsonReadyString& operator=(JsonReadyString&& other) noexcept;

        bool needsJsonEscape() const noexcept;
        
        boost::string_view string() const noexcept;
        boost::string_view quotedString() const noexcept;

        friend void swap(JsonReadyString& first, JsonReadyString& second) noexcept
        {
            for (size_t i = 0; i < StackSize; ++i)
            {
                std::swap(first.buffer.stack[i], second.buffer.stack[i]);
            }
            std::swap(first.buffer.external, second.buffer.external);
            std::swap(first.size, second.size);
        }

    private:
        const char* stringStart() const noexcept;

        union
        {
            char stack[StackSize];
            char* external = nullptr;
        } buffer;
        /**
         * Includes the size of the quotes if present.
         * Positive if no escaping needed, negative if escaping needed.
         */
        int32_t size = 0;
    };

    bool isEscapeNeeded(const char* value, size_t length);

    template <size_t StackSize>
    JsonReadyString<StackSize>::JsonReadyString(boost::string_view source)
    {
        char* destination = buffer.stack;
        auto sourceSize = source.size();

        if (isEscapeNeeded(source.data(), sourceSize))
        {
            if (sourceSize > StackSize)
            {
                destination = buffer.external = new char[sourceSize];
            }
            size = -static_cast<decltype(size)>(sourceSize);
        }
        else
        {
            //also use add quotes
            auto destinationSize = sourceSize + 2;
            size = static_cast<decltype(size)>(destinationSize);

            if (destinationSize > StackSize)
            {
                destination = buffer.external = new char[destinationSize];
            }            
            *destination = '"';
            destination[destinationSize - 1] = '"';
            destination += 1;
        }

        std::copy(source.begin(), source.end(), destination);
    }

    template <size_t StackSize>
    JsonReadyString<StackSize>::~JsonReadyString()
    {
        if ((static_cast<size_t>(std::abs(size)) > StackSize) && buffer.external)
        {
            delete[] buffer.external;
            buffer.external = nullptr;
        }
    }

    template <size_t StackSize>
    JsonReadyString<StackSize>::JsonReadyString(const JsonReadyString& other)
    {
        size = other.size;
        const char* source = other.buffer.stack;
        char* destination = buffer.stack;
        auto absSize = std::abs(size);

        if (absSize > StackSize)
        {
            destination = buffer.external = new char[absSize];
            source = other.buffer.external;
        }

        std::copy(source, source + absSize, destination);
    }

    template <size_t StackSize>
    JsonReadyString<StackSize>::JsonReadyString(JsonReadyString&& other) noexcept
    {
        std::swap(*this, other);
    }

    template <size_t StackSize>
    JsonReadyString<StackSize>& JsonReadyString<StackSize>::operator=(JsonReadyString other)
    {
        std::swap(*this, other);
        return *this;
    }

    template <size_t StackSize>
    JsonReadyString<StackSize>& JsonReadyString<StackSize>::operator=(JsonReadyString&& other) noexcept
    {
        std::swap(*this, other);
        return *this;
    }

    template <size_t StackSize>
    bool JsonReadyString<StackSize>::needsJsonEscape() const noexcept
    {
        return size < 0;
    }

    template <size_t StackSize>
    boost::string_view JsonReadyString<StackSize>::string() const noexcept
    {
        auto strStart = stringStart();
        auto strSize = static_cast<boost::string_view::size_type>(std::abs(size));

        if ( ! needsJsonEscape())
        {
            //exclude quotes
            strStart += 1;
            strSize -= 2;
        }

        return boost::string_view(strStart, strSize);
    }

    template <size_t StackSize>
    boost::string_view JsonReadyString<StackSize>::quotedString() const noexcept
    {
        if (needsJsonEscape())
        {
            assert(false);
            return{};
        }

        return boost::string_view(stringStart(), static_cast<boost::string_view::size_type>(std::abs(size)));
    }

    template <size_t StackSize>
    const char* JsonReadyString<StackSize>::stringStart() const noexcept
    {
        return (std::abs(size) <= StackSize) ? buffer.stack : buffer.external;
    }
}
