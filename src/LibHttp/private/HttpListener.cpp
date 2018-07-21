/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "HttpListener.h"
#include "FixedSizeBuffer.h"
#include "HttpParser.h"
#include "HttpResponseBuilder.h"
#include "HttpRouter.h"
#include "TimeoutManager.h"

#include <atomic>

using namespace Http;

typedef FixedSizeBufferPool<Buffer::ReadBufferSize> ReadBufferPoolType;
typedef FixedSizeBufferPool<Buffer::ReadBufferSize>::LeasedBufferType ReadBufferType;
typedef FixedSizeBufferPool<Buffer::WriteBufferSize> WriteBufferPoolType;
typedef FixedSizeBufferPool<Buffer::WriteBufferSize>::LeasedBufferType WriteBufferType;
typedef ReadWriteBufferArray<Buffer::ReadBufferSize, Buffer::MaximumBuffersForRequestBody> RequestBodyBufferType;
typedef ReadWriteBufferArray<Buffer::WriteBufferSize, Buffer::MaximumBuffersForResponse> ResponseBufferType;

static void writeToBuffer(const char* data, size_t size, void* state)
{
    reinterpret_cast<ResponseBufferType*>(state)->write(data, size);
}

static void closeSocket(boost::asio::ip::tcp::socket& socket)
{
    try
    {
        socket.shutdown(boost::asio::socket_base::shutdown_both);
        socket.close();
    }
    catch (...)
    {
    }
}

struct ConnectionInfo
{
    boost::asio::ip::tcp::socket* socket;
    boost::asio::io_service::strand* strand;

    bool operator==(ConnectionInfo other) const
    {
        return (socket == other.socket) && (strand == other.strand);
    }
};

inline size_t hash_value(const ConnectionInfo& value)
{
    return std::hash<boost::asio::ip::tcp::socket*>{}(value.socket)
        ^ std::hash<boost::asio::io_service::strand*>{}(value.strand);
}

struct HttpListener::HttpConnection final : private boost::noncopyable
{
    explicit HttpConnection(HttpListener& listener, boost::asio::ip::tcp::socket&& socket,
                            boost::asio::io_service::strand&& strand, ReadBufferType&& headerBuffer,
                            ReadBufferPoolType& readBufferPool, WriteBufferPoolType& writeBufferPool,
                            TimeoutManager<ConnectionInfo>& timeoutManager, bool trustIpFromXForwardedFor)
        : listener_(listener), socket_(std::move(socket)), strand_(std::move(strand)),
          headerBuffer_(std::move(headerBuffer)), requestBodyBuffer_(readBufferPool), responseBuffer_(writeBufferPool),
          responseBuilder_(writeToBuffer, &responseBuffer_), timeoutManager_(timeoutManager),
          trustIpFromXForwardedFor_(trustIpFromXForwardedFor),
          parser_(headerBuffer_->data, headerBuffer_->size, Buffer::MaxRequestBodyLength,
              [](auto buffer, auto size, auto state)
              {
                  return reinterpret_cast<HttpConnection*>(state)->onReadBody(buffer, size);
              }, this)
    {
        timeoutManager_.addExpireIn({ &socket_, &strand_ }, timeoutManager.defaultTimeout());
    }

    auto& socket()
    {
        return socket_;
    }

    auto& strand()
    {
        return strand_;
    }

    void startReading()
    {
        strand_.post([this]()
        {
            boost::asio::async_read(socket_, boost::asio::buffer(readBuffer_), boost::asio::transfer_at_least(1),
                strand_.wrap(HttpConnectionReadCallback{ this }));
        });
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
            auto& request = parser_.mutableRequest();
            keepConnectionAlive_ = parser_.request().keepConnectionAlive;

            //add references to request content buffers
            for (const auto buffer : requestBodyBuffer_.constBufferWrapper())
            {
                if (request.nrOfRequestContentBuffers >= request.requestContentBuffers.size())
                {
                    break;
                }
                request.requestContentBuffers[request.nrOfRequestContentBuffers++] =
                    HttpStringView(boost::asio::buffer_cast<const char*>(buffer), boost::asio::buffer_size(buffer));
            }

            //add remote endpoint
            if (trustIpFromXForwardedFor_)
            {
                static thread_local char nullTerminatedAddressBuffer[128];
                auto xForwardedFor = request.headers[Http::Request::X_Forwarded_For];
                const auto toCopy = std::min(std::size(nullTerminatedAddressBuffer) - 1, xForwardedFor.size());
                std::copy(xForwardedFor.data(), xForwardedFor.data() + toCopy, nullTerminatedAddressBuffer);
                nullTerminatedAddressBuffer[toCopy] = 0;

                boost::system::error_code parseAddressCode;
                const auto address = boost::asio::ip::address::from_string(nullTerminatedAddressBuffer, parseAddressCode);
                if ( ! parseAddressCode)
                {
                    request.remoteAddress = address;
                }
            }
            else
            {
                boost::system::error_code getAddressCode;
                auto remoteEndpoint = socket_.remote_endpoint(getAddressCode);
                if ( ! getAddressCode)
                {
                    request.remoteAddress = remoteEndpoint.address();
                }
            }

            listener_.router().forward(request, responseBuilder_);

            if ((0 == responseBuffer_.size()) || responseBuffer_.notEnoughRoom())
            {
                writeStatusCode(HttpStatusCode::Internal_Server_Error);
            }
            else
            {
                strand_.post([this]()
                {
                    boost::asio::async_write(socket_, responseBuffer_.constBufferWrapper(), boost::asio::transfer_all(),
                                             strand_.wrap(HttpConnectionResponseWrittenCallback{ this }));
                });
            }
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

        strand_.post([this, responseSize]()
        {
            boost::asio::async_write(socket_, boost::asio::buffer(readBuffer_.data(), responseSize),
                                     boost::asio::transfer_all(), strand_.wrap(HttpConnectionResponseWrittenCallback{ this }));
        });
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
            requestBodyBuffer_.reset();
            responseBuffer_.reset();
            responseBuilder_.reset();
            startReading();
        }
        else
        {
            strand_.wrap([this]()
            {
                closeSocket(socket_);
            });
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
    boost::asio::io_service::strand strand_;
    ReadBufferType headerBuffer_;
    RequestBodyBufferType requestBodyBuffer_;
    std::array<char, 1024> readBuffer_{};
    ResponseBufferType responseBuffer_;
    HttpResponseBuilder responseBuilder_;
    TimeoutManager<ConnectionInfo>& timeoutManager_;
    bool keepConnectionAlive_ = false;
    bool trustIpFromXForwardedFor_ = false;
    Parser parser_;
};

struct HttpListener::HttpListenerImpl
{
    struct TimeoutCheckCallback
    {
        void operator()(const boost::system::error_code&)
        {
            if (impl->nrOfCurrentlyOpenConnections > 0)
            {
                impl->timeoutManager.checkTimeout();
            }
            impl->timeoutTimer.expires_at(impl->timeoutTimer.expires_at()
                    + boost::posix_time::seconds(CheckTimeoutEverySeconds));

            impl->timeoutTimer.async_wait(*this);
        }
        HttpListenerImpl* impl;
    };

    HttpListenerImpl(Configuration&& config, HttpRouter& router, boost::asio::io_service& ioService)
        : config(std::move(config)), router(router), ioService(ioService), acceptor(ioService),
        socket(ioService), strand(ioService),
        timeoutTimer(ioService, boost::posix_time::seconds(CheckTimeoutEverySeconds)),
        connectionPool(config.numberOfReadBuffers),
        timeoutManager([](auto info) { HttpListenerImpl::closeConnection(info); }, this->config.connectionTimeoutSeconds)
    {
        readBuffers = std::make_unique<ReadBufferPoolType>(
                static_cast<size_t>(config.numberOfReadBuffers));
        writeBuffers = std::make_unique<WriteBufferPoolType>(
                static_cast<size_t>(config.numberOfWriteBuffers));

        timeoutTimer.async_wait(TimeoutCheckCallback{ this });
    }

    static void closeConnection(ConnectionInfo info)
    {
        if (info.socket && info.strand)
        {
            auto socket = info.socket;
            info.strand->post([socket]()
            {
                closeSocket(*socket);
            });
        }
    }

    Configuration config;
    HttpRouter& router;

    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;
    boost::asio::io_service::strand strand;
    boost::asio::deadline_timer timeoutTimer;

    constexpr static int CheckTimeoutEverySeconds = 1;

    std::unique_ptr<ReadBufferPoolType> readBuffers;
    std::unique_ptr<WriteBufferPoolType> writeBuffers;
    FixedSizeObjectPool<HttpConnection> connectionPool;
    std::atomic<std::int64_t> nrOfCurrentlyOpenConnections{0};
    TimeoutManager<ConnectionInfo> timeoutManager;
};

HttpListener::HttpListener(Configuration config, HttpRouter& router, boost::asio::io_service& ioService)
    : impl_(new HttpListenerImpl(std::move(config), router, ioService))
{
}

HttpListener::~HttpListener()
{
    delete impl_;
}

void HttpListener::startListening()
{
    boost::asio::ip::tcp::endpoint endpoint
    {
        boost::asio::ip::address::from_string(impl_->config.listenIPAddress),
        impl_->config.listenPort
    };

    impl_->acceptor.open(endpoint.protocol());
    impl_->acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
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

        impl_->timeoutTimer.cancel(ec);
    });
}

HttpRouter& HttpListener::router()
{
    return impl_->router;
}

namespace Http
{
    struct HttpListenerOnAcceptCallback
    {
        void operator()(const boost::system::error_code& ec)
        {
            listener->onAccept(ec);
        }
        HttpListener* listener;
    };
}

void HttpListener::startAccept()
{
    impl_->acceptor.async_accept(impl_->socket, HttpListenerOnAcceptCallback{ this });
}

void HttpListener::onAccept(const boost::system::error_code& ec)
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

    auto closeConnection = true;

    auto headerBuffer = impl_->readBuffers->leaseBuffer();

    if (headerBuffer)
    {
        auto connection = impl_->connectionPool.getObject(*this, std::move(impl_->socket), std::move(impl_->strand),
                                                          std::move(headerBuffer),
                                                          *impl_->readBuffers, *impl_->writeBuffers,
                                                          impl_->timeoutManager, impl_->config.trustIpFromXForwardedFor);
        if (nullptr != connection)
        {
            closeConnection = false;
            impl_->nrOfCurrentlyOpenConnections += 1;
            connection->startReading();
        }
    }

    if (closeConnection)
    {
        impl_->strand.post([this]()
        {
            closeSocket(impl_->socket);
        });
    }

    startAccept();
}

void HttpListener::release(HttpConnection* connection)
{
    if (connection)
    {
        impl_->timeoutManager.remove({ &connection->socket(), &connection->strand() });
    }
    impl_->connectionPool.returnObject(connection);
    impl_->nrOfCurrentlyOpenConnections -= 1;
}
