#pragma once

#include <cstdint>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

namespace Http
{
    class HttpRouter;

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
        
        explicit HttpListener(Configuration config, HttpRouter& router, boost::asio::io_service& ioService);
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
        * The response can occupy at most this amount of buffers
        */
        static constexpr size_t MaximumBuffersForResponse = 256;
        /**
         * Each response can request multiple buffers 
         */
        static constexpr size_t WriteBufferSize = 8192;

        void startListening();
        void stopListening();

        HttpRouter& router();

    private:
        
        void startAccept();
        friend struct HttpListenerOnAcceptCallback;
        void onAccept(const boost::system::error_code& ec);

        friend struct HttpConnection;        
        void release(HttpConnection* connection);
        
        struct HttpListenerImpl;
        HttpListenerImpl* impl_;
    };
}
