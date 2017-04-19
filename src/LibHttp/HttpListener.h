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
            int_fast16_t connectionTimeoutSeconds;
        };
        
        explicit HttpListener(Configuration config, HttpRouter& router, boost::asio::io_service& ioService);
        ~HttpListener();

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
