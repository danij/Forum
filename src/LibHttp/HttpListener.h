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
         * Each request is only allocated 1 buffer
         */
        static constexpr size_t ReadBufferSize = 4096;
        /**
         * Each response can request multiple buffers 
         */
        static constexpr size_t WriteBufferSize = 16384;
        /**
         * Maximum number of bytes which the HTTP header is allowed to ocupy
         */
        static constexpr size_t MaxHeaderSize = 16384;

        void startListening();
        void stopListening();

    private:
        void startAccept();
        void onAccept(boost::system::error_code ec);
        
        struct HttpListenerImpl;
        HttpListenerImpl* impl_;
    };
}
