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

#include <iostream>
#include <memory>
#include <cassert>
#include <cstring>

#include "JsonWriter.h"

using namespace std;
using namespace Json;

JsonWriter::JsonWriter(ostream& stream) : _stream(stream)
{
   _state.push({ false, false, false });
}

JsonWriter::~JsonWriter()
{
}

JsonWriter& JsonWriter::null()
{
   addCommaIfNeeded();
   _stream << "null";
   return *this;
}

JsonWriter& JsonWriter::startArray()
{
   addCommaIfNeeded();
   _stream << '[';
   _state.push({ true, false, false });
   return *this;
}

JsonWriter& JsonWriter::endArray()
{
   _stream << ']';
   _state.pop();
   return *this;
}

JsonWriter& JsonWriter::startObject()
{
   addCommaIfNeeded();
   _stream << '{';
   _state.push({ true, false, false });
   return *this;
}

JsonWriter& JsonWriter::endObject()
{
   _stream << '}';
   _state.pop();
   return *this;
}

JsonWriter& JsonWriter::newProperty(const char* name)
{
   addCommaIfNeeded();
   writeEscapedString(name);
   _stream << ':';
   _state.top().propertyNameAdded = true;
   return *this;
}

JsonWriter& JsonWriter::newProperty(const string& name)
{
   addCommaIfNeeded();
   writeEscapedString(name.c_str(), name.length());
   _stream << ':';
   _state.top().propertyNameAdded = true;
   return *this;
}

JsonWriter& JsonWriter::newPropertyWithSafeName(const char* name)
{
   addCommaIfNeeded();
   _stream << '"' << name << "\":";
   _state.top().propertyNameAdded = true;
   return *this;
}

JsonWriter& JsonWriter::newPropertyWithSafeName(const string& name)
{
   addCommaIfNeeded();
   _stream << '"' << name << "\":";
   _state.top().propertyNameAdded = true;
   return *this;
}

void JsonWriter::addCommaIfNeeded()
{
   auto& state = _state.top();
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
            _stream << ',';
         }
      }
      else
      {
         state.commaRequired = true;
      }
   }
}

static const unsigned char toEscape[128] = {
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x62, 0x74, 0x6E, 0xFF, 0x66, 0x72, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char hexDigits[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

#define MAX_STACK_BUFFER 60000

void JsonWriter::writeEscapedString(const char* value, size_t length)
{
   if (length == 0)
   {
      length = strlen(value);
   }

   _stream << '"';
   if (length > 0)
   {
      const int toEscapeLength = sizeof(toEscape) / sizeof(toEscape[0]);
      const int bufferMultiplier = 6;//in the worst case, something like \u005C is needed for each character
      char stackBuffer[MAX_STACK_BUFFER] = { 0 };
      char* buffer = stackBuffer;
      unique_ptr<char> toDelete;

      if ((length * bufferMultiplier) >= MAX_STACK_BUFFER)
      {
         toDelete.reset(buffer = new char[length * bufferMultiplier + 1]);
         memset(buffer, 0, (length * bufferMultiplier + 1) * sizeof(char));
      }

      int dstIndex = 0;
      for (size_t i = 0; i < length; i++)
      {
         const auto& c = static_cast<unsigned char>(value[i]);
         if (c >= toEscapeLength)
         {
            buffer[dstIndex++] = c;
         }
         else
         {
            auto r = toEscape[c];
            if (r)
            {
               //escape needed
               if (r < 0xFF)
               {
                  //we have a special character for the escape
                  buffer[dstIndex++] = '\\';
                  buffer[dstIndex++] = r;
               }
               else
               {
                  //we must use the six-character sequence
                  //simplified as we only escape control characters this way
                  buffer[dstIndex++] = '\\';
                  buffer[dstIndex++] = 'u';
                  buffer[dstIndex++] = '0';
                  buffer[dstIndex++] = '0';
                  buffer[dstIndex++] = hexDigits[c / 16];
                  buffer[dstIndex++] = hexDigits[c % 16];
               }
            }
            else
            {
               //no escape needed
               buffer[dstIndex++] = c;
            }
         }
      }

      _stream << buffer;
   }
   _stream << '"';
}
