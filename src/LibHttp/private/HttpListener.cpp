#include "HttpListener.h"
#include "ConstantBuffer.h"
#include "HttpParser.h"
#include "HttpResponseBuilder.h"

#include <boost/pool/pool_alloc.hpp>

using namespace Http;

typedef ConstantBufferManager<HttpListener::ReadBufferSize>::LeasedBufferType ReadBufferType;
typedef ConstantBufferManager<HttpListener::WriteBufferSize>::LeasedBufferType WriteBufferType;

struct HttpConnection final : std::enable_shared_from_this<HttpConnection>, private boost::noncopyable
{
    explicit HttpConnection(boost::asio::ip::tcp::socket&& socket, ReadBufferType&& headerBuffer)
        : socket_(std::move(socket)), headerBuffer_(std::move(headerBuffer)),
        parser_(headerBuffer_->data.data(), headerBuffer_->data.size(),
            [](auto buffer, auto size, auto state)
            {
                return reinterpret_cast<HttpConnection*>(state)->onReadBody(buffer, size);
            }, this)
    {}

    void startReading()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(readBuffer_), boost::asio::transfer_at_least(1),
            [p = shared_from_this()](auto& ec, auto bytesTransfered) { p->onRead(ec, bytesTransfered); });
    }

    void onRead(const boost::system::error_code& ec, size_t bytesTransfered)
    {
        if (ec == boost::asio::error::eof)
        {
            //connection closed, nothing more to do regarding this connection
            return;
        }
        if (ec)
        {
            auto msg = ec.message();
            //TODO: handle error
            return;
        }
        if (0 == bytesTransfered)
        {
            return;
        }
        
        if (parser_.process(readBuffer_.data(), bytesTransfered) == Parser::ParseResult::INVALID_INPUT)
        {
            //invalid input
            //reuse the input buffer for sending the error code
            auto responseSize = buildSimpleResponseFromStatusCode(parser_.errorCode(),
                parser_.request().versionMajor, parser_.request().versionMinor, readBuffer_.data());

            boost::asio::async_write(socket_, boost::asio::buffer(readBuffer_.data(), responseSize), 
                                     boost::asio::transfer_all(),
                                     [p = shared_from_this()](auto& ec, auto bytesTransfered)
                                     {
                                         p->onResponseWritten(ec, bytesTransfered);
                                     });
            return;
        }

        if (parser_ == Parser::ParseResult::FINISHED)
        {
            //finished reading everything needed for the current request, so process it
            auto& request = parser_.request();
            //TODO get reply and send it            
            boost::asio::async_write(socket_, boost::asio::buffer(static_cast<void*>(nullptr), 0), boost::asio::transfer_all(),
                [p = shared_from_this()](auto& ec, auto bytesTransfered) { p->onResponseWritten(ec, bytesTransfered); });
        }
        else
        {
            startReading();            
        }
    }

    bool onReadBody(char* buffer, size_t size)
    {
        return true;
    }

    void onResponseWritten(const boost::system::error_code& ec, size_t bytesTransfered)
    {
        if (ec)
        {
            //connection closed or error occured, nothing more to do regarding this connection
            return;
        }

        if (parser_.request().keepConnectionAlive)
        {
            parser_.reset();
            startReading();
        }
        else
        {
            try
            {
                socket_.shutdown(boost::asio::socket_base::shutdown_both);
                socket_.close();
            }
            catch (...)
            {
            }
        }
    }

private:
    boost::asio::ip::tcp::socket socket_;
    ReadBufferType headerBuffer_;
    std::array<char, 1024> readBuffer_;
    Parser parser_;
};

struct HttpListener::HttpListenerImpl
{
    HttpListenerImpl(Configuration&& config, boost::asio::io_service& ioService)
        : config(std::move(config)), ioService(ioService), acceptor(ioService), socket(ioService)
    {
        readBuffers = std::make_unique<ConstantBufferManager<ReadBufferSize>>(
                static_cast<size_t>(config.numberOfReadBuffers));
        writeBuffers = std::make_unique<ConstantBufferManager<WriteBufferSize>>(
                static_cast<size_t>(config.numberOfWriteBuffers));
    }

    Configuration config;

    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;

    std::unique_ptr<ConstantBufferManager<ReadBufferSize>> readBuffers;
    std::unique_ptr<ConstantBufferManager<WriteBufferSize>> writeBuffers;
};

HttpListener::HttpListener(Configuration config, boost::asio::io_service& ioService)
    : impl_(new HttpListenerImpl(std::move(config), ioService))
{
}

HttpListener::~HttpListener()
{
    if (impl_)
    {
        delete impl_;
    }
}

void HttpListener::startListening()
{
    //TODO: error checking
    boost::asio::ip::tcp::endpoint endpoint
    { 
        boost::asio::ip::address::from_string(impl_->config.listenIPAddress), 
        impl_->config.listenPort 
    };

    impl_->acceptor.open(endpoint.protocol());
    impl_->acceptor.bind(endpoint);
    impl_->acceptor.listen();

    startAccept();
}

void HttpListener::stopListening()
{
    impl_->ioService.dispatch([&] 
    { 
        boost::system::error_code ec;
        impl_->acceptor.close(ec);
    });
}

void HttpListener::startAccept()
{
    impl_->acceptor.async_accept(impl_->socket, [&](auto ec) { this->onAccept(ec); });
}

void HttpListener::onAccept(boost::system::error_code ec)
{
    if ( ! impl_->acceptor.is_open())
    {
        return;
    }
    if (ec)
    {
        //TODO: handle error
        return;
    }

    auto headerBuffer = impl_->readBuffers->leaseBuffer();

    if ( ! headerBuffer)
    {
        //no more buffers available
        return;
    }
    
    //TODO: better memory management based on reuse
    auto connection = std::make_shared<HttpConnection>(std::move(impl_->socket), std::move(headerBuffer));
    connection->startReading();

    startAccept();
}
