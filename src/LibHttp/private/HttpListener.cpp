#include "HttpListener.h"
#include "FixedSizeBuffer.h"
#include "HttpParser.h"
#include "HttpResponseBuilder.h"

#include <boost/pool/pool_alloc.hpp>

using namespace Http;

typedef FixedSizeBufferPool<HttpListener::ReadBufferSize> ReadBufferManagerType;
typedef FixedSizeBufferPool<HttpListener::ReadBufferSize>::LeasedBufferType ReadBufferType;
typedef FixedSizeBufferPool<HttpListener::WriteBufferSize>::LeasedBufferType WriteBufferType;
typedef ReadWriteBufferArray<HttpListener::ReadBufferSize, HttpListener::MaximumBuffersForRequestBody> RequestBodyBufferType;

struct Http::HttpConnection final : private boost::noncopyable
{
    explicit HttpConnection(HttpListener& listener, boost::asio::ip::tcp::socket&& socket, ReadBufferType&& headerBuffer, 
                            ReadBufferManagerType& readBufferManager)
        : listener_(listener), socket_(std::move(socket)), headerBuffer_(std::move(headerBuffer)), 
          requestBodyBuffer_(readBufferManager), parser_(headerBuffer_->data, headerBuffer_->size, 
          HttpListener::MaxRequestBodyLength,
              [](auto buffer, auto size, auto state)
              {
                  return reinterpret_cast<HttpConnection*>(state)->onReadBody(buffer, size);
              }, this)
    {}

    void startReading()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(readBuffer_), boost::asio::transfer_at_least(1),
            HttpConnectionReadCallback{ this });
    }

    void onRead(const boost::system::error_code& ec, size_t bytesTransfered)
    {
        if (ec == boost::asio::error::eof)
        {
            //connection closed, nothing more to do regarding this connection
            listener_.release(this);
            return;
        }
        if (ec)
        {
            auto msg = ec.message();
            //TODO: handle error
            listener_.release(this);
            return;
        }
        if (0 == bytesTransfered)
        {
            listener_.release(this);
            return;
        }
        
        if (parser_.process(readBuffer_.data(), bytesTransfered) == Parser::ParseResult::INVALID_INPUT)
        {
            //invalid input
            writeStatusCode(parser_.errorCode());
            return;
        }

        if (parser_ == Parser::ParseResult::FINISHED)
        {
            //finished reading everything needed for the current request, so process it
            auto& request = parser_.request();
            //TODO get reply and send it         
            
            keepConnectionAlive_ = parser_.request().keepConnectionAlive;
            writeStatusCode(HttpStatusCode::OK);

            //boost::asio::async_write(socket_, boost::asio::buffer(static_cast<void*>(nullptr), 0), boost::asio::transfer_all(),
            //    HttpConnectionResponseWrittenCallback{ this });
        }
        else
        {
            startReading();            
        }
    }

    bool onReadBody(const char* buffer, size_t size)
    {
        return requestBodyBuffer_.write(buffer, size);
    }

    void writeStatusCode(HttpStatusCode code)
    {
        //reuse the input buffer for sending the error code
        auto responseSize = buildSimpleResponseFromStatusCode(code,
            parser_.request().versionMajor, parser_.request().versionMinor, readBuffer_.data());

        boost::asio::async_write(socket_, boost::asio::buffer(readBuffer_.data(), responseSize),
                                 boost::asio::transfer_all(), HttpConnectionResponseWrittenCallback{ this });
    }

    void onResponseWritten(const boost::system::error_code& ec, size_t bytesTransfered)
    {
        if (ec)
        {
            //connection closed or error occured, nothing more to do regarding this connection
            listener_.release(this);
            return;
        }

        if (keepConnectionAlive_)
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
            listener_.release(this);
        }
    }

private:

    struct HttpConnectionReadCallback
    {
        void operator()(const boost::system::error_code& ec, size_t bytesTransfered)
        {
            connection->onRead(ec, bytesTransfered);
        }
        HttpConnection* connection;
    };

    struct HttpConnectionResponseWrittenCallback
    {
        void operator()(const boost::system::error_code& ec, size_t bytesTransfered)
        {
            connection->onResponseWritten(ec, bytesTransfered);
        }
        HttpConnection* connection;
    };

    HttpListener& listener_;
    boost::asio::ip::tcp::socket socket_;
    ReadBufferType headerBuffer_;
    RequestBodyBufferType requestBodyBuffer_;
    std::array<char, 1024> readBuffer_;
    Parser parser_;
    bool keepConnectionAlive_ = false;
};

struct HttpListener::HttpListenerImpl
{
    HttpListenerImpl(Configuration&& config, boost::asio::io_service& ioService)
        : config(std::move(config)), ioService(ioService), acceptor(ioService), socket(ioService),
          connectionPool(config.numberOfReadBuffers)
    {
        readBuffers = std::make_unique<FixedSizeBufferPool<ReadBufferSize>>(
                static_cast<size_t>(config.numberOfReadBuffers));
        writeBuffers = std::make_unique<FixedSizeBufferPool<WriteBufferSize>>(
                static_cast<size_t>(config.numberOfWriteBuffers));
    }

    Configuration config;

    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;

    std::unique_ptr<FixedSizeBufferPool<ReadBufferSize>> readBuffers;
    std::unique_ptr<FixedSizeBufferPool<WriteBufferSize>> writeBuffers;
    FixedSizeObjectPool<HttpConnection> connectionPool;
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

    if (headerBuffer)
    {
        auto connection = impl_->connectionPool.getObject(*this, std::move(impl_->socket), std::move(headerBuffer),
                                                          *impl_->readBuffers);
        if (nullptr != connection)
        {
            connection->startReading();
        }
    }
    startAccept();
}

void HttpListener::release(HttpConnection* connection)
{
    impl_->connectionPool.returnObject(connection);
}
