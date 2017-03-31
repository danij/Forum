#pragma once

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <beast/http.hpp>

#include <memory>

namespace Forum
{
    namespace Network
    {
        struct ConnectionState final : std::enable_shared_from_this<ConnectionState>, private boost::noncopyable
        {
            boost::asio::ip::tcp::socket socket;
            beast::streambuf streamBuffer;
            beast::http::request<beast::http::string_body> request;
            beast::http::response<beast::http::string_body> response;

            explicit ConnectionState(boost::asio::ip::tcp::socket&& socket) : socket(std::move(socket))
            {}
        };

        class HttpListener final : private boost::noncopyable
        {
        public:
            explicit HttpListener(boost::asio::io_service& ioService);

            void startListening();
            void stopListening();

        private:
            void startAccept();
            void onAccept(boost::system::error_code ec);
            void onRead(ConnectionState& state, boost::system::error_code ec);
            
            boost::asio::io_service& ioService_;
            boost::asio::ip::tcp::acceptor acceptor_;
            boost::asio::ip::tcp::socket socket_;
        };
    }
}
