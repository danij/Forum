#pragma once

#include "HttpConstants.h"

#include <cstddef>

#include <boost/noncopyable.hpp>

namespace Http
{
    /**
     * Constructs a minimal HTTP response for the specified status code
     * @param code Status code which will be written both as a number and as a description
     * @param majorVersion Major version sent in the reply
     * @param minorVersion Minor version sent in the reply
     * @param buffer A large enough buffer (no bounds are checked)
     * @returns The number of bytes written
     */
    size_t buildSimpleResponseFromStatusCode(HttpStatusCode code, int_fast8_t majorVersion, int_fast8_t minorVersion,
                                             char* buffer);

    class HttpResponseBuilder final : private boost::noncopyable
    {
    public:
        typedef void (*WriteFn)(const char* data, size_t size, void* state);

        HttpResponseBuilder(WriteFn writeFn, void* writeState);

        void writeResponseCode(int majorVersion, int minorVersion, HttpStatusCode code);
        void writeHeader(StringView name, StringView value);
        void writeHeader(StringView name, int value);

        template<size_t NameSize>
        void writeHeader(const char (&name)[NameSize], StringView value)
        {
            writeHeader(StringView(name, NameSize - 1), value);
        }

        template<size_t NameSize>
        void writeHeader(const char(&name)[NameSize], int value)
        {
            writeHeader(StringView(name, NameSize - 1), value);
        }

        template<size_t NameSize, size_t ValueSize>
        void writeHeader(const char(&name)[NameSize], const char (&value)[ValueSize])
        {
            writeHeader(StringView(name, NameSize - 1), StringView(value, ValueSize - 1));
        }

        void writeBody(const char* value, size_t length);
        void writeBodyAndContentLength(const char* value, size_t length);

    private:
        void write(const char* data, size_t size)
        {
            writeFn_(data, size, writeState_);
        }

        void write(StringView view)
        {
            write(view.data(), view.size());
        }

        template<size_t Size>
        void write(const char (&data)[Size])
        {
            write(data, Size - 1);
        }

        enum class ProtocolState
        {
            NothingWritten,
            ResponseCodeWritten,
            BodyWritten,
        };

        ProtocolState protocolState_ = ProtocolState::NothingWritten;
        WriteFn writeFn_;
        void* writeState_;
    };
}
