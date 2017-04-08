#pragma once

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

#include <cstdint>
#include <string>

namespace Http
{
    class HttpListener final : private boost::noncopyable
    {
    public:
        struct Configuration
        {
            int_fast16_t numberOfIOServiceThreads;
            int_fast32_t numberOfReadBuffers;
            int_fast32_t numberOfWriteBuffers;
            std::string listenIPAddress;
            uint16_t listenPort;
        };

        explicit HttpListener(Configuration config, boost::asio::io_service& ioService);
        ~HttpListener();

        /**
         * Each request needs at least one buffer; the request header must fit into one buffer to avoid fragmentation
         */
        static constexpr size_t ReadBufferSize = 4096;
        /**
         * The body of a request can occupy at most this amount of buffers
         */
        static constexpr size_t MaximumBuffersForRequestBody = 100;
        /**
         * The maximum size of a request body
         */
        static constexpr size_t MaxRequestBodyLength = ReadBufferSize * MaximumBuffersForRequestBody;
        /**
         * Each response can request multiple buffers 
         */
        static constexpr size_t WriteBufferSize = 16384;

        void startListening();
        void stopListening();

    private:
        void startAccept();
        void onAccept(boost::system::error_code ec);
        
        struct HttpListenerImpl;
        HttpListenerImpl* impl_;
    };
}
