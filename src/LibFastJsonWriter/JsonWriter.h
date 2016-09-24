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

#include <functional>
#include <iostream>
#include <stack>
#include <string>

namespace Json
{
    class JsonWriter
    {
    public:
        explicit JsonWriter(std::ostream& stream);

        JsonWriter(const JsonWriter&) = delete;

        virtual ~JsonWriter();

        JsonWriter& operator=(const JsonWriter& other) = delete;

        JsonWriter& null();

        JsonWriter& startArray();

        JsonWriter& endArray();

        JsonWriter& startObject();

        JsonWriter& endObject();

        JsonWriter& newProperty(const char* name);

        JsonWriter& newProperty(const std::string& name);

        JsonWriter& newPropertyWithSafeName(const char* name);

        JsonWriter& newPropertyWithSafeName(const std::string& name);

    protected:

        void addCommaIfNeeded();
        void writeEscapedString(const char* value, size_t length = 0);

        template<typename T>
        friend JsonWriter& operator<<(JsonWriter& writer, const T& value);
        friend JsonWriter& operator<<(JsonWriter& writer, bool value);
        friend JsonWriter& operator<<(JsonWriter& writer, const char* value);
        friend JsonWriter& operator<<(JsonWriter& writer, const std::string& value);

        struct State
        {
            bool enumerationStarted;
            bool commaRequired;
            bool propertyNameAdded;
        };

        std::ostream& _stream;
        std::stack<State> _state;
    };

    template <typename T>
    inline JsonWriter& operator<<(JsonWriter& writer, const T& value)
    {
        writer.addCommaIfNeeded();
        writer._stream << value;
        return writer;
    }

    inline JsonWriter& operator<<(JsonWriter& writer, bool value)
    {
        writer.addCommaIfNeeded();
        writer._stream << (value ? "true" : "false");
        return writer;
    }

    inline JsonWriter& operator<<(JsonWriter& writer, const char* value)
    {
        writer.addCommaIfNeeded();
        writer.writeEscapedString(value);
        return writer;
    }

    inline JsonWriter& operator<<(JsonWriter& writer, const std::string& value)
    {
        writer.addCommaIfNeeded();
        writer.writeEscapedString(value.c_str(), value.length());
        return writer;
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
    inline JsonWriter& writeArray(JsonWriter& writer, ForwardIterator begin, ForwardIterator end)
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

    template<typename T1, typename T2>
    JsonWriter& operator<<(JsonWriter& writer, JsonWriterManipulatorWithTwoParams<T1, T2> manipulator)
    {
        return manipulator.function(writer, manipulator.argument1, manipulator.argument2);
    }

    template<typename StringType, typename ValueType>
    inline JsonWriter& _property(JsonWriter& writer, const StringType& name, const ValueType& value)
    {
        writer.newProperty(name) << value;
        return writer;
    }

    template<typename StringType, typename ValueType>
    inline JsonWriter& _propertySafe(JsonWriter& writer, const StringType& name, const ValueType& value)
    {
        writer.newPropertyWithSafeName(name) << value;
        return writer;
    }

    template<typename StringType, typename ValueType>
    inline JsonWriterManipulatorWithTwoParams<StringType, ValueType> property(const StringType& name,
                                                                              const ValueType& value)
    {
        return JsonWriterManipulatorWithTwoParams<StringType, ValueType>(_property<StringType, ValueType>, name, value);
    }

    template<typename StringType, typename ValueType>
    inline JsonWriterManipulatorWithTwoParams<StringType, ValueType> propertySafeName(const StringType& name,
                                                                                      const ValueType& value)
    {
        return JsonWriterManipulatorWithTwoParams<StringType, ValueType>(_propertySafe<StringType, ValueType>, name,
                                                                         value);
    }

    template<typename ForwardIterator>
    struct IteratorPair
    {
        ForwardIterator begin;
        ForwardIterator end;

        IteratorPair(ForwardIterator _begin, ForwardIterator _end) : begin(_begin), end(_end)
        { }
    };

    template<typename ForwardIterator>
    JsonWriter& operator<<(JsonWriter& writer, const IteratorPair<ForwardIterator>& pair)
    {
        return writeArray(writer, pair.begin, pair.end);
    }

    template<typename ForwardIterator>
    inline IteratorPair<ForwardIterator> enumerate(ForwardIterator begin, ForwardIterator end)
    {
        return IteratorPair<ForwardIterator>(begin, end);
    }
}
