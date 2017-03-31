#include "HttpListener.h"
#include "Configuration.h"

#include <beast/http.hpp>

using namespace Forum;
using namespace Forum::Network;

HttpListener::HttpListener(boost::asio::io_service& ioService)
    : ioService_(ioService), acceptor_(ioService_), socket_(ioService_)
{
}

void HttpListener::startListening()
{
    auto config = Configuration::getGlobalConfig();
    //TODO: error checking
    boost::asio::ip::tcp::endpoint endpoint
    { 
        boost::asio::ip::address::from_string(config->service.listenIPAddress), 
        config->service.listenPort 
    };

    acceptor_.open(endpoint.protocol());
    acceptor_.bind(endpoint);
    acceptor_.listen();

    startAccept();
}

void HttpListener::stopListening()
{
    ioService_.dispatch([&] 
    { 
        boost::system::error_code ec;
        acceptor_.close(ec);
    });
}

void HttpListener::startAccept()
{
    acceptor_.async_accept(socket_, [&](auto ec) { this->onAccept(ec); });
}

void HttpListener::onAccept(boost::system::error_code ec)
{
    if ( ! acceptor_.is_open())
    {
        return;
    }
    if (ec)
    {
        //TODO: handle error
        return;
    }
    auto acceptedSocket(std::move(socket_));

    //TODO: better memory management based on reuse
    auto state = std::make_shared<ConnectionState>(std::move(acceptedSocket));
   
    beast::http::async_read(state->socket, state->streamBuffer, state->request,
        [this, state](auto readEc) { onRead(*state, readEc); });

    startAccept();
}

void HttpListener::onRead(ConnectionState& state, boost::system::error_code ec)
{
    if (ec)
    {
        //TODO: handle error
        return;
    }

    state.response.version = state.request.version;
    state.response.status = 200;
    state.response.reason = beast::http::reason_string(state.response.status);
    state.response.body = "Hello World";
    state.response.fields.insert("Content-Type", "text/plain");
    prepare(state.response);

    beast::http::async_write(state.socket, state.response, [s = state.shared_from_this()](auto ec)
    {
        if (ec)
        {
            //TODO: handle error
        }
    });
}
