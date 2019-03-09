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

#include <array>
#include <limits>
#include <cstdint>

#include "JsonReadyString.h"
#include "StringBuffer.h"

#define JSON_RAW_PROP(x) "\"" x "\":"
#define JSON_RAW_PROP_COMMA(x) ",\"" x "\":"

#define JSON_WRITE_FIRST_PROP(writer, key, value) writer.newPropertyRaw(JSON_RAW_PROP(key)) << value
#define JSON_WRITE_PROP(writer, key, value) writer.newPropertyRaw(JSON_RAW_PROP_COMMA(key)) << value

namespace Json
{
    namespace Detail
    {
        inline void writeChar(StringBuffer& stringBufferOutput, const char value)
        {
            stringBufferOutput.write(value);
        }

        inline void writeString(StringBuffer& stringBufferOutput, const char* value, const size_t size)
        {
            stringBufferOutput.write(value, size);
        }

        template<size_t Size>
        void writeString(StringBuffer& stringBufferOutput, const char(&value)[Size])
        {
            //ignore null terminator
            stringBufferOutput.writeFixed<Size - 1>(value);
        }

        template <>
        inline void writeString<1>(StringBuffer& /*stringBufferOutput*/, const char(&/*value*/)[0 + 1])
        {
            //ignore null terminator
        }

        template <>
        inline void writeString<2>(StringBuffer& stringBufferOutput, const char(&value)[1 + 1])
        {
            //ignore null terminator
            writeChar(stringBufferOutput, value[0]);
        }

        template <>
        inline void writeString<3>(StringBuffer& stringBufferOutput, const char(&value)[2 + 1])
        {
            //ignore null terminator
            stringBufferOutput.writeFixed<2>(value);
        }

        template <>
        inline void writeString<5>(StringBuffer& stringBufferOutput, const char(&value)[4 + 1])
        {
            //ignore null terminator
            stringBufferOutput.writeFixed<4>(value);
        }

        template <>
        inline void writeString<9>(StringBuffer& stringBufferOutput, const char(&value)[8 + 1])
        {
            //ignore null terminator
            stringBufferOutput.writeFixed<8>(value);
        }
    }

    const char HexDigits[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    template<typename OutputBuffer>
    void escapeString(const char* value, const size_t length, OutputBuffer& destination)
    {
        static_assert((ToEscapeLength - 1) == std::numeric_limits<unsigned char>::max());

        if ( ! isEscapeNeeded(value, length)) {
            Detail::writeString(destination, value, length);
            return;
        }

        char twoCharEscapeBuffer[2+1] = { '\\', 0, 0 };
        char sixCharEscapeBuffer[6+1] = { '\\', 'u', '0', '0', 0, 0, 0 };

        auto directWriteFrom = value;
        const auto endValue = value + length;

        while (value < endValue)
        {
            const auto c = static_cast<unsigned char>(*value);
            const auto r = ToEscape[c];
            if (r)
            {
                //escape needed
                if (directWriteFrom < value)
                {
                    //flush previous characters that don't require escaping
                    Detail::writeString(destination, directWriteFrom, value - directWriteFrom);
                }
                directWriteFrom = value + 1; //skip the current character as it needs escaping
                if (r < 0xFF)
                {
                    //we have a special character for the escape
                    twoCharEscapeBuffer[1] = static_cast<char>(r);
                    Detail::writeString(destination, twoCharEscapeBuffer);
                }
                else
                {
                    //we must use the six-character sequence
                    //simplified as we only escape control characters this way
                    sixCharEscapeBuffer[4] = HexDigits[c / 16];
                    sixCharEscapeBuffer[5] = HexDigits[c % 16];
                    Detail::writeString(destination, sixCharEscapeBuffer);
                }
            }
            ++value;
        }

        if (directWriteFrom < value)
        {
            //write remaining characters that don't require escaping
            Detail::writeString(destination, directWriteFrom, value - directWriteFrom);
        }
    }

    template<typename OutputBuffer>
    class JsonWriterBase final
    {
    public:
        explicit JsonWriterBase(OutputBuffer& stringBuffer) : stringBufferOutput_(stringBuffer)
        {
            pushState({ 0, 0, 0 });
        }
        ~JsonWriterBase() = default;

        JsonWriterBase(const JsonWriterBase&) = delete;
        JsonWriterBase(JsonWriterBase&&) = default;

        JsonWriterBase& operator=(const JsonWriterBase& other) = delete;
        JsonWriterBase& operator=(JsonWriterBase&& other) = default;

        constexpr static int MaxStateDepth = 32;

        JsonWriterBase& null()
        {
            if (isCommaNeeded())
            {
                writeString(",null");
            }
            else
            {
                writeString("null");
            }
            return *this;
        }

        JsonWriterBase& startArray()
        {
            if (isCommaNeeded())
            {
                writeString(",[");
            }
            else
            {
                writeChar('[');
            }
            pushState({ 1, 0, 0 });
            return *this;
        }

        JsonWriterBase& endArray()
        {
            writeChar(']');

            popState();
            return *this;
        }

        JsonWriterBase& startObject()
        {
            if (isCommaNeeded())
            {
                writeString(",{");
            }
            else
            {
                writeChar('{');
            }
            pushState({ 1, 0, 0 });
            return *this;
        }

        JsonWriterBase& endObject()
        {
            writeChar('}');

            popState();
            return *this;
        }

        JsonWriterBase& newProperty(const char* name)
        {
            writeEscapedString(name);
            writeChar(':');

            peekState().propertyNameAdded = 1;
            return *this;
        }

        JsonWriterBase& newProperty(std::string_view name)
        {
            writeEscapedString(name.data(), name.length());
            writeChar(':');

            peekState().propertyNameAdded = 1;
            return *this;
        }

        JsonWriterBase& newPropertyWithSafeName(const char* name, const size_t length)
        {
            if (isCommaNeeded())
            {
                writeString(",\"");
            }
            else
            {
                writeChar('"');
            }
            writeString(name, length);
            writeString("\":");

            peekState().propertyNameAdded = 1;
            return *this;
        }

        template<size_t Length>
        JsonWriterBase& newPropertyWithSafeName(const char(&name)[Length])
        {
            //ignore null terminator
            return newPropertyWithSafeName(name, Length - 1);
        }

        JsonWriterBase& newPropertyWithSafeName(std::string_view name)
        {
            return newPropertyWithSafeName(name.data(), name.size());
        }

        template<size_t Length>
        JsonWriterBase& newPropertyRaw(const char(&name)[Length])
        {
            writeString<Length>(name);

            peekState().propertyNameAdded = 1;
            return *this;
        }

        JsonWriterBase& writeEscapedString(const char* value, size_t length = 0)
        {
            if (isCommaNeeded())
            {
                writeString(",\"");
            }
            else
            {
                writeChar('"');
            }

            if (length == 0)
            {
                length = strlen(value);
            }

            escapeString(value, length, stringBufferOutput_);

            writeChar('"');
            return *this;
        }

        JsonWriterBase& writeEscapedString(std::string_view value)
        {
            return writeEscapedString(value.data(), value.size());
        }

        JsonWriterBase& writeSafeString(const char* value, const size_t length)
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

        JsonWriterBase& writeSafeString(std::string_view value)
        {
            return writeSafeString(value.data(), value.size());
        }

        JsonWriterBase& operator<<(const bool value)
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

        JsonWriterBase& operator<<(const char value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const unsigned char value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const short value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const unsigned short value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const int value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const unsigned int value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const unsigned long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const long long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const unsigned long long value)
        {
            return writeNumber(value, isCommaNeeded());
        }

        JsonWriterBase& operator<<(const char* value)
        {
            return writeEscapedString(value);
        }

        JsonWriterBase& operator<<(const std::string& value)
        {
            return writeEscapedString(value);
        }

        JsonWriterBase& operator<<(const std::string_view value)
        {
            return writeEscapedString(value);
        }

        template<typename T>
        JsonWriterBase& operator<<(const T* value)
        {
            return value ? (*this << *value) : null();
        }

        JsonWriterBase& operator<<(JsonWriterBase& (* manipulator)(JsonWriterBase&))
        {
            return manipulator(*this);
        }

        template <size_t StackSize, typename Derived, typename SizeType>
        JsonWriterBase& operator<<(const JsonReadyStringBase<StackSize, Derived, SizeType>& jsonReadyString)
        {
            if (jsonReadyString.needsJsonEscape())
            {
                return writeEscapedString(jsonReadyString.string());
            }
            //string already contains quotes
            if (isCommaNeeded())
            {
                writeChar(',');
            }
            auto string = jsonReadyString.quotedString();
            writeString(string.data(), string.size());
            return *this;
        }

    protected:

        bool isCommaNeeded()
        {
            auto& state = peekState();
            //if (state.enumerationStarted)
            //{
            //    if (state.commaRequired)
            //    {
            //        if (state.propertyNameAdded)
            //        {
            //            state.propertyNameAdded = false;
            //        }
            //        else
            //        {
            //            result = true;
            //        }
            //    }
            //    else
            //    {
            //        state.commaRequired = true;
            //    }
            //}

            auto result = state.enumerationStarted && state.commaRequired && ! state.propertyNameAdded;
            state.commaRequired = state.enumerationStarted | state.commaRequired;
            state.propertyNameAdded = 0;

            return result;
        }

        constexpr static int maxDigitsOfNumber(const int numberOfBytes) noexcept
        {
            return 1 == numberOfBytes ? 3
                : 2 == numberOfBytes ? 5
                : 4 == numberOfBytes ? 10
                : 8 == numberOfBytes ? 20
                : 16 == numberOfBytes ? 39
                : 128;
        }

        template<typename T>
        typename std::enable_if<std::numeric_limits<T>::is_integer, JsonWriterBase&>::type
            writeNumber(T value, const bool includeComma)
        {
            //minimum integer values cannot be negated
            if constexpr (std::numeric_limits<T>::is_signed)
            {
                if (std::numeric_limits<T>::min() == value)
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
            }

            char digitsBuffer[maxDigitsOfNumber(sizeof(T)) + 3]; //extra space for sign and comma

            char* bufferEnd = digitsBuffer + (sizeof(digitsBuffer) / sizeof(digitsBuffer[0]));
            char* digit = bufferEnd - 1;

            bool addSign = false;
            if constexpr (std::numeric_limits<T>::is_signed)
            {
                if (value < 0)
                {
                    addSign = true;
                    value = -value;
                }
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
        }

        template<size_t Size>
        void writeString(const char(&value)[Size])
        {
            Detail::writeString<Size>(stringBufferOutput_, value);
        }

        void writeString(std::string_view& value)
        {
            writeString(value.data(), value.size());
        }

        void writeString(const char* value, size_t size)
        {
            Detail::writeString(stringBufferOutput_, value, size);
        }

        void writeChar(char value)
        {
            Detail::writeChar(stringBufferOutput_, value);
        }

        struct State
        {
            uint8_t enumerationStarted :1;
            uint8_t commaRequired      :1;
            uint8_t propertyNameAdded  :1;
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

        OutputBuffer& stringBufferOutput_;

        std::array<State, MaxStateDepth> stateStack_;
        int stateIndex_ = -1;
    };

    typedef JsonWriterBase<StringBuffer> JsonWriter;

    template<typename OutputBuffer>
    JsonWriterBase<OutputBuffer>& nullObj(JsonWriterBase<OutputBuffer>& writer)
    {
        writer.null();
        return writer;
    }

    template<typename OutputBuffer>
    JsonWriterBase<OutputBuffer>& objStart(JsonWriterBase<OutputBuffer>& writer)
    {
        writer.startObject();
        return writer;
    }

    template<typename OutputBuffer>
    JsonWriterBase<OutputBuffer>& objEnd(JsonWriterBase<OutputBuffer>& writer)
    {
        writer.endObject();
        return writer;
    }

    template<typename OutputBuffer>
    JsonWriterBase<OutputBuffer>& arrayStart(JsonWriterBase<OutputBuffer>& writer)
    {
        writer.startArray();
        return writer;
    }

    template<typename OutputBuffer>
    JsonWriterBase<OutputBuffer>& arrayEnd(JsonWriterBase<OutputBuffer>& writer)
    {
        writer.endArray();
        return writer;
    }

    template<typename OutputBuffer, typename ForwardIterator>
    JsonWriterBase<OutputBuffer>& writeArray(JsonWriterBase<OutputBuffer>& writer, ForwardIterator begin,
                                             ForwardIterator end)
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

    template<typename OutputBuffer, typename ForwardIterator, typename ActionType>
    JsonWriterBase<OutputBuffer>& writeArray(JsonWriterBase<OutputBuffer>& writer, ForwardIterator begin,
                                             ForwardIterator end, ActionType preWriteAction)
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

    template<typename OutputBuffer, typename T1, typename T2>
    struct JsonWriterManipulatorWithTwoParams
    {
        typedef JsonWriterBase<OutputBuffer>& (* Function)(JsonWriterBase<OutputBuffer>&, const T1&, const T2&);

        Function function;
        const T1& argument1;
        const T2& argument2;

        JsonWriterManipulatorWithTwoParams(const Function func, const T1& arg1, const T2& arg2) :
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

    template<typename T1, size_t T1Size, typename T2, size_t T2Size>
    struct JsonWriterManipulatorPropertySafeNameArrayValueArray
    {
        const T1(&argument1)[T1Size];
        const T2(&argument2)[T2Size];

        JsonWriterManipulatorPropertySafeNameArrayValueArray(const T1(&arg1)[T1Size], const T2(&arg2)[T2Size]) :
                argument1(arg1), argument2(arg2)
        { }
    };

    template<typename OutputBuffer, typename T1, typename T2>
    JsonWriterBase<OutputBuffer>& operator<<(JsonWriterBase<OutputBuffer>& writer,
                                             JsonWriterManipulatorWithTwoParams<OutputBuffer, T1, T2> manipulator)
    {
        return manipulator.function(writer, manipulator.argument1, manipulator.argument2);
    }

    template<typename OutputBuffer, typename T1, size_t T1Size, typename T2>
    JsonWriterBase<OutputBuffer>& operator<<(JsonWriterBase<OutputBuffer>& writer,
                                             JsonWriterManipulatorPropertySafeNameArray<T1, T1Size, T2> manipulator)
    {
        return writer.newPropertyWithSafeName(manipulator.argument1) << manipulator.argument2;
    }

    template<typename OutputBuffer, typename T1, size_t T1Size, typename T2, size_t T2Size>
    JsonWriterBase<OutputBuffer>& operator<<(JsonWriterBase<OutputBuffer>& writer,
                                             JsonWriterManipulatorPropertySafeNameArrayValueArray<T1, T1Size, T2, T2Size> manipulator)
    {
        return writer.newPropertyWithSafeName(manipulator.argument1)
                << std::string_view(manipulator.argument2, T2Size - 1);
    }

    template<typename OutputBuffer, typename StringType, typename ValueType>
    JsonWriterBase<OutputBuffer>& _property(JsonWriterBase<OutputBuffer>& writer, const StringType& name,
                                            const ValueType& value)
    {
        return writer.newProperty(name) << value;
    }

    template<typename OutputBuffer, typename StringType, typename ValueType>
    JsonWriterBase<OutputBuffer>& _propertySafe(JsonWriterBase<OutputBuffer>& writer, const StringType& name,
                                                const ValueType& value)
    {
        writer.newPropertyWithSafeName(name) << value;
        return writer;
    }

    template<typename OutputBuffer, typename StringType, typename ValueType>
    JsonWriterManipulatorWithTwoParams<OutputBuffer, StringType, ValueType> property(const StringType& name,
                                                                                     const ValueType& value)
    {
        return JsonWriterManipulatorWithTwoParams<OutputBuffer, StringType, ValueType>(
                _property<StringType, ValueType>, name, value);
    }

    template<typename OutputBuffer, typename StringType, typename ValueType>
    JsonWriterManipulatorWithTwoParams<OutputBuffer, StringType, ValueType> propertySafeName(const StringType& name,
                                                                                             const ValueType& value)
    {
        return JsonWriterManipulatorWithTwoParams<OutputBuffer, StringType, ValueType>(
                _propertySafe<StringType, ValueType>, name, value);
    }

    template<size_t StringSize, typename ValueType>
    JsonWriterManipulatorPropertySafeNameArray<const char, StringSize, ValueType> propertySafeName(const char(&name)[StringSize],
                                                                                                   const ValueType& value)
    {
        return JsonWriterManipulatorPropertySafeNameArray<const char, StringSize, ValueType>(name, value);
    }

    template<size_t StringSize, size_t ValueSize>
    JsonWriterManipulatorPropertySafeNameArrayValueArray<const char, StringSize, const char, ValueSize> propertySafeName(
            const char(&name)[StringSize], const char(&value)[ValueSize])
    {
        return JsonWriterManipulatorPropertySafeNameArrayValueArray<const char, StringSize, const char, ValueSize>(name, value);
    }

    template<typename ForwardIterator>
    struct IteratorPair
    {
        ForwardIterator begin;
        ForwardIterator end;

        IteratorPair(ForwardIterator _begin, ForwardIterator _end) : begin(std::move(_begin)), end(std::move(_end))
        { }
    };

    template<typename OutputBuffer, typename ForwardIterator>
    JsonWriterBase<OutputBuffer>& operator<<(JsonWriterBase<OutputBuffer>& writer,
                                             const IteratorPair<ForwardIterator>& pair)
    {
        return writeArray(writer, pair.begin, pair.end);
    }

    template<typename ForwardIterator>
    IteratorPair<ForwardIterator> enumerate(ForwardIterator begin, ForwardIterator end)
    {
        return IteratorPair<ForwardIterator>(begin, end);
    }
}
