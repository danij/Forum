#pragma once

#include "StringContainer.h"

#include <cassert>
#include <cstdint>

#include <boost/utility/string_view.hpp>
#include <boost/functional/hash.hpp>

namespace Json
{
    struct SizeWithBool
    {
        uint32_t boolean : 1;
        uint32_t size    :31;

        SizeWithBool() : boolean(0), size(0) {}
        SizeWithBool(size_t size) : boolean(0), size(static_cast<decltype(SizeWithBool::size)>(size)) {}

        SizeWithBool(const SizeWithBool&) = default;
        SizeWithBool& operator=(const SizeWithBool&) = default;

        explicit operator size_t() const noexcept
        {
            return static_cast<size_t>(size);
        }

        SizeWithBool& operator=(size_t value) noexcept
        {
            size = static_cast<decltype(size)>(value);
            return *this;
        }

        explicit operator bool() const noexcept
        {
            return boolean != 0;
        }

        SizeWithBool& operator=(bool value) noexcept
        {
            boolean = value ? 1 : 0;
            return *this;
        }
    };

    template<size_t StackSize, typename Derived, typename SizeType = SizeWithBool>
    class JsonReadyStringBase
    {
    public:
        explicit JsonReadyStringBase(boost::string_view source);
        JsonReadyStringBase(const JsonReadyStringBase&) = default;
        JsonReadyStringBase(JsonReadyStringBase&&) noexcept = default;

        JsonReadyStringBase& operator=(const JsonReadyStringBase&) = default;
        JsonReadyStringBase& operator=(JsonReadyStringBase&&) noexcept = default;

        bool operator==(const JsonReadyStringBase&) const noexcept;

        bool needsJsonEscape() const noexcept;

        boost::string_view string() const noexcept;
        boost::string_view quotedString() const noexcept;

    protected:
        StringContainer<StackSize, SizeType> container_;
    };

    bool isEscapeNeeded(const char* value, size_t length);

    template<size_t StackSize>
    class JsonReadyString : public JsonReadyStringBase<StackSize, JsonReadyString<StackSize>>
    {
    public:
        explicit JsonReadyString(boost::string_view source);
        JsonReadyString(const JsonReadyString&) = default;
        JsonReadyString(JsonReadyString&&) noexcept = default;

        JsonReadyString& operator=(const JsonReadyString&) = default;
        JsonReadyString& operator=(JsonReadyString&&) noexcept = default;

        size_t getExtraSize() const noexcept;

        static size_t extraBytesNeeded(boost::string_view source);
    };

    template <size_t StackSize, typename Derived, typename SizeType>
    JsonReadyStringBase<StackSize, Derived, SizeType>::JsonReadyStringBase(boost::string_view source)
    {
        auto sourceSize = source.size();
        auto bytesNeeded = sourceSize;

        auto escapeNeeded = isEscapeNeeded(source.data(), sourceSize);

        if ( ! escapeNeeded)
        {
            bytesNeeded += 2; //start and end quotes
        }

        bytesNeeded += Derived::extraBytesNeeded(source);

        container_.resize(bytesNeeded);

        auto& sizeInfo = container_.size();
        sizeInfo = escapeNeeded;

        auto destination = *container_;

        if ( ! escapeNeeded)
        {
            *destination++ = '"';
            destination[sourceSize] = '"';
        }
        std::copy(source.begin(), source.end(), destination);
    }

    template <size_t StackSize, typename Derived, typename SizeType>
    bool JsonReadyStringBase<StackSize, Derived, SizeType>::operator==(const JsonReadyStringBase& other) const noexcept
    {
        return container_ == other.container_;
    }

    template <size_t StackSize, typename Derived, typename SizeType>
    bool JsonReadyStringBase<StackSize, Derived, SizeType>::needsJsonEscape() const noexcept
    {
        return static_cast<bool>(container_.size());
    }

    template <size_t StackSize, typename Derived, typename SizeType>
    boost::string_view JsonReadyStringBase<StackSize, Derived, SizeType>::string() const noexcept
    {
        auto strStart = *container_;
        auto extraSize = static_cast<const Derived*>(this)->getExtraSize();

        auto strSize = static_cast<boost::string_view::size_type>(container_.size());
        assert(strSize > extraSize);

        strSize -= extraSize;

        if ( ! needsJsonEscape())
        {
            //exclude quotes
            strStart += 1;
            strSize -= 2;
        }

        return boost::string_view(strStart, strSize);
    }

    template <size_t StackSize, typename Derived, typename SizeType>
    boost::string_view JsonReadyStringBase<StackSize, Derived, SizeType>::quotedString() const noexcept
    {
        if (needsJsonEscape())
        {
            assert(false);
            return{};
        }

        auto extraSize = static_cast<const Derived*>(this)->getExtraSize();
        auto strSize = static_cast<boost::string_view::size_type>(container_.size());

        assert(strSize > extraSize);
        strSize -= extraSize;

        return boost::string_view(*container_, strSize);
    }

    template <size_t StackSize>
    JsonReadyString<StackSize>::JsonReadyString(boost::string_view source)
        : JsonReadyStringBase<StackSize, JsonReadyString<StackSize>>(source)
    {
    }

    template <size_t StackSize>
    size_t JsonReadyString<StackSize>::extraBytesNeeded(boost::string_view source)
    {
        return 0;
    }

    template <size_t StackSize>
    size_t JsonReadyString<StackSize>::getExtraSize() const noexcept
    {
        return 0;
    }
}

namespace std
{
    template<size_t StackSize, typename Derived, typename SizeType>
    struct hash<Json::JsonReadyStringBase<StackSize, Derived, SizeType>>
    {
        size_t operator()(const Json::JsonReadyStringBase<StackSize, Derived, SizeType>& value) const
        {
            return boost::hash<boost::string_view>{}(value.string());
        }
    };
}
