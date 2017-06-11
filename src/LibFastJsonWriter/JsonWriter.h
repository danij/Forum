/*
Fast class for writing Json documents
Copyright (C) 2015 dani.user@gmail.com

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

#include <iostream>
#include <array>
#include <limits>
#include <string>

#include "StringBuffer.h"

#include <boost/utility/string_view.hpp>

namespace Json
{
    namespace Detail
    {
        inline void writeChar(std::ostream* streamOutput, StringBuffer* stringBufferOutput, char value)
        {
            if (stringBufferOutput)
            {
                stringBufferOutput->write(value);
            }
            else
            {
                *streamOutput << value;
            }
        }

        inline void writeString(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char* value, size_t size)
        {
            if (stringBufferOutput)
            {
                stringBufferOutput->write(value, size);
            }
            else
            {
                streamOutput->write(value, size);
            }
        }

        template<size_t Size>
        void writeString(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char(&value)[Size])
        {
            //ignore null terminator
            writeString(streamOutput, stringBufferOutput, value, Size - 1);
        }

        template <>
        inline void writeString<1>(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char(&value)[0 + 1])
        {
            //ignore null terminator
        }

        template <>
        inline void writeString<2>(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char(&value)[1 + 1])
        {
            //ignore null terminator
            writeChar(streamOutput, stringBufferOutput, value[0]);
        }

        template <>
        inline void writeString<3>(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char(&value)[2 + 1])
        {
            //ignore null terminator
            if (stringBufferOutput)
            {
                stringBufferOutput->write(*reinterpret_cast<const uint16_t*>(value));
            }
            else
            {
                writeString(streamOutput, stringBufferOutput, value, 2);
            }
        }

        template <>
        inline void writeString<5>(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char(&value)[4 + 1])
        {
            //ignore null terminator
            if (stringBufferOutput)
            {
                stringBufferOutput->write(*reinterpret_cast<const uint32_t*>(value));
            }
            else
            {
                writeString(streamOutput, stringBufferOutput, value, 4);
            }
        }

        template <>
        inline void writeString<9>(std::ostream* streamOutput, StringBuffer* stringBufferOutput, const char(&value)[8 + 1])
        {
            //ignore null terminator
            if (stringBufferOutput)
            {
                stringBufferOutput->write(*reinterpret_cast<const uint64_t*>(value));
            }
            else
            {
                writeString(streamOutput, stringBufferOutput, value, 8);
            }
        }
    }

    class JsonWriter final
    {
    public:
        explicit JsonWriter(std::ostream& stream);
        //explicit JsonWriter(std::string& stringBuffer);
        explicit JsonWriter(StringBuffer& stringBuffer);

        JsonWriter(const JsonWriter&) = delete;
        JsonWriter(JsonWriter&&) = default;

        JsonWriter& operator=(const JsonWriter& other) = delete;
        JsonWriter& operator=(JsonWriter&& other) = default;

        constexpr static int MaxStateDepth = 30;

        JsonWriter& null();

        JsonWriter& startArray();

        JsonWriter& endArray();

        JsonWriter& startObject();

        JsonWriter& endObject();

        JsonWriter& newProperty(const char* name);

        JsonWriter& newProperty(const std::string& name);

        JsonWriter& newPropertyWithSafeName(const char* name, size_t length);

        template<size_t Length>
        JsonWriter& newPropertyWithSafeName(const char(&name)[Length])
        {
            //ignore null terminator
            return newPropertyWithSafeName(name, Length - 1);
        }

        JsonWriter& newPropertyWithSafeName(const std::string& name);

        JsonWriter& writeEscapedString(const char* value, size_t length = 0);

        JsonWriter& writeEscapedString(boost::string_view value)
        {
            return writeEscapedString(value.data(), value.size());
        }

        JsonWriter& writeSafeString(const char* value, size_t length)
        {
            if (isCommaNeeded())
            {
                writeString(",\"");
            }
            else
            {
                writeChar('"');
            }
            writeString(value, length);
            writeChar('"');
            return *this;
        }

        JsonWriter& writeSafeString(boost::string_view value)
        {
            return writeSafeString(value.data(), value.size());
        }

        JsonWriter& operator<<(bool value)
        {
            if (isCommaNeeded())
            {
                if (value) writeString(",true"); else writeString(",false");
            }
            else
            {
                if (value) writeString("true"); else writeString("false");
            }
            return *this;
        }

        JsonWriter& operator<<(char value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(unsigned char value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(short value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(unsigned short value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(int value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(unsigned int value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(unsigned long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(long long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriter& operator<<(unsigned long long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

    protected:

        bool isCommaNeeded()
        {
            bool result = false;
            auto& state = peekState();
            if (state.enumerationStarted)
            {
                if (state.commaRequired)
                {
                    if (state.propertyNameAdded)
                    {
                        state.propertyNameAdded = false;
                    }
                    else
                    {
                        result = true;
                    }
                }
                else
                {
                    state.commaRequired = true;
                }
            }
            return result;
        }

        constexpr static int maxDigitsOfNumber(int numberOfBytes) noexcept
        {
            return 1 == numberOfBytes ? 3
                : 2 == numberOfBytes ? 5
                : 4 == numberOfBytes ? 10
                : 8 == numberOfBytes ? 20
                : 16 == numberOfBytes ? 39
                : 128;
        }

        template<typename T>
        typename std::enable_if<std::numeric_limits<T>::is_integer, JsonWriter&>::type
            writeNumber(T value, bool includeComma)
        {
            //minimum integer values cannot be negated
            if (std::numeric_limits<T>::is_signed && std::numeric_limits<T>::min() == value)
            {
                switch (sizeof(T))
                {
                case 1:
                    writeString("-128");
                    break;
                case 2:
                    writeString("-32768");
                    break;
                case 4:
                    writeString("-2147483648");
                    break;
                case 8:
                    writeString("-9223372036854775808");
                    break;
                default:
                    break;
                }
                return *this;
            }

            char digitsBuffer[maxDigitsOfNumber(sizeof(T)) + 3]; //extra space for sign and comma

            char* bufferEnd = digitsBuffer + (sizeof(digitsBuffer) / sizeof(digitsBuffer[0]));
            char* digit = bufferEnd - 1;

            bool addSign = false;
            if (value < 0)
            {
                addSign = true;
                value = -value;
            }

            *digit-- = (value % 10) + '0';
            while (value > 9)
            {
                value /= 10;
                *digit-- = (value % 10) + '0';
            }
            if (addSign)
            {
                *digit-- = '-';
            }
            if (includeComma)
            {
                *digit-- = ',';
            }

            ++digit;
            writeString(digit, bufferEnd - digit);

            return *this;
        };

        template<size_t Size>
        void writeString(const char(&value)[Size])
        {
            Detail::writeString<Size>(streamOutput_, stringBufferOutput_, value);
        }

        void writeString(const std::string& value)
        {
            writeString(value.c_str(), value.size());
        }

        void writeString(const char* value, size_t size)
        {
            Detail::writeString(streamOutput_, stringBufferOutput_, value, size);
        }

        void writeChar(char value)
        {
            Detail::writeChar(streamOutput_, stringBufferOutput_, value);
        }

        friend JsonWriter& operator<<(JsonWriter& writer, const char* value);
        friend JsonWriter& operator<<(JsonWriter& writer, const std::string& value);

        struct State
        {
            bool enumerationStarted;
            bool commaRequired;
            bool propertyNameAdded;
        };

        State& peekState()
        {
            return stateStack_.at(stateIndex_);
        }

        void popState()
        {
            --stateIndex_;
        }

        void pushState(State state)
        {
            stateStack_.at(++stateIndex_) = state;
        }

        std::ostream* streamOutput_ = nullptr;
        StringBuffer* stringBufferOutput_ = nullptr;

        std::array<State, MaxStateDepth> stateStack_;
        int stateIndex_ = -1;
    };

    template <typename T>
    JsonWriter& operator<<(JsonWriter& writer, const T* value)
    {
        return value ? (writer << *value) : writer.null();
    }

    inline JsonWriter& operator<<(JsonWriter& writer, const char* value)
    {
        return writer.writeEscapedString(value);
    }

    inline JsonWriter& operator<<(JsonWriter& writer, const std::string& value)
    {
        return writer.writeEscapedString(value.c_str(), value.length());
    }

    inline JsonWriter& operator<<(JsonWriter& writer, boost::string_view value)
    {
        return writer.writeEscapedString(value);
    }

    inline JsonWriter& operator<<(JsonWriter& writer, JsonWriter& (* manipulator)(JsonWriter&))
    {
        return manipulator(writer);
    }

    inline JsonWriter& nullObj(JsonWriter& writer)
    {
        writer.null();
        return writer;
    }

    inline JsonWriter& objStart(JsonWriter& writer)
    {
        writer.startObject();
        return writer;
    }

    inline JsonWriter& objEnd(JsonWriter& writer)
    {
        writer.endObject();
        return writer;
    }

    inline JsonWriter& arrayStart(JsonWriter& writer)
    {
        writer.startArray();
        return writer;
    }

    inline JsonWriter& arrayEnd(JsonWriter& writer)
    {
        writer.endArray();
        return writer;
    }

    template<typename ForwardIterator>
    JsonWriter& writeArray(JsonWriter& writer, ForwardIterator begin, ForwardIterator end)
    {
        writer.startArray();
        while (begin != end)
        {

            operator<<(writer, *begin);
            ++begin;
        }
        writer.endArray();
        return writer;
    }

    template<typename ForwardIterator, typename ActionType>
    JsonWriter& writeArray(JsonWriter& writer, ForwardIterator begin, ForwardIterator end, ActionType preWriteAction)
    {
        writer.startArray();
        while (begin != end)
        {
            preWriteAction(*begin);
            operator<<(writer, *begin);
            ++begin;
        }
        writer.endArray();
        return writer;
    }

    template<typename T1, typename T2>
    struct JsonWriterManipulatorWithTwoParams
    {
        typedef JsonWriter& (* Function)(JsonWriter&, const T1&, const T2&);

        Function function;
        const T1& argument1;
        const T2& argument2;

        JsonWriterManipulatorWithTwoParams(Function func, const T1& arg1, const T2& arg2) :
                function(func), argument1(arg1), argument2(arg2)
        { }
    };

    template<typename T1, size_t T1Size, typename T2>
    struct JsonWriterManipulatorPropertySafeNameArray
    {
        const T1(&argument1)[T1Size];
        const T2& argument2;

        JsonWriterManipulatorPropertySafeNameArray(const T1(&arg1)[T1Size], const T2& arg2) :
                argument1(arg1), argument2(arg2)
        { }
    };

    template<typename T1, typename T2>
    JsonWriter& operator<<(JsonWriter& writer, JsonWriterManipulatorWithTwoParams<T1, T2> manipulator)
    {
        return manipulator.function(writer, manipulator.argument1, manipulator.argument2);
    }

    template<typename T1, size_t T1Size, typename T2>
    JsonWriter& operator<<(JsonWriter& writer, JsonWriterManipulatorPropertySafeNameArray<T1, T1Size, T2> manipulator)
    {
        writer.newPropertyWithSafeName<T1Size>(manipulator.argument1) << manipulator.argument2;
        return writer;
    }

    template<typename StringType, typename ValueType>
    JsonWriter& _property(JsonWriter& writer, const StringType& name, const ValueType& value)
    {
        writer.newProperty(name) << value;
        return writer;
    }

    template<typename StringType, typename ValueType>
    JsonWriter& _propertySafe(JsonWriter& writer, const StringType& name, const ValueType& value)
    {
        writer.newPropertyWithSafeName(name) << value;
        return writer;
    }

    template<typename StringType, typename ValueType>
    JsonWriterManipulatorWithTwoParams<StringType, ValueType> property(const StringType& name,
                                                                       const ValueType& value)
    {
        return JsonWriterManipulatorWithTwoParams<StringType, ValueType>(_property<StringType, ValueType>, name, value);
    }

    template<typename StringType, typename ValueType>
    JsonWriterManipulatorWithTwoParams<StringType, ValueType> propertySafeName(const StringType& name,
                                                                               const ValueType& value)
    {
        return JsonWriterManipulatorWithTwoParams<StringType, ValueType>(_propertySafe<StringType, ValueType>, name,
                                                                         value);
    }

    template<size_t StringSize, typename ValueType>
    JsonWriterManipulatorPropertySafeNameArray<const char, StringSize, ValueType> propertySafeName(const char(&name)[StringSize],
                                                                                                   const ValueType& value)
    {
        return JsonWriterManipulatorPropertySafeNameArray<const char, StringSize, ValueType>(name, value);
    }

    template<typename ForwardIterator>
    struct IteratorPair
    {
        ForwardIterator begin;
        ForwardIterator end;

        IteratorPair(ForwardIterator _begin, ForwardIterator _end) : begin(std::move(_begin)), end(std::move(_end))
        { }
    };

    template<typename ForwardIterator>
    JsonWriter& operator<<(JsonWriter& writer, const IteratorPair<ForwardIterator>& pair)
    {
        return writeArray(writer, pair.begin, pair.end);
    }

    template<typename ForwardIterator>
    IteratorPair<ForwardIterator> enumerate(ForwardIterator begin, ForwardIterator end)
    {
        return IteratorPair<ForwardIterator>(begin, end);
    }

}
