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

#pragma once

#include <cstdint>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

namespace Http
{
    class HttpRouter;

    class HttpListener final : boost::noncopyable
    {
    public:
        struct Configuration
        {
            int_fast32_t numberOfReadBuffers;
            int_fast32_t numberOfWriteBuffers;
            std::string listenIPAddress;
            uint16_t listenPort;
            int_fast16_t connectionTimeoutSeconds;
            bool trustIpFromXForwardedFor;
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

        struct HttpConnection;
        friend struct HttpConnection;
        void release(HttpConnection* connection);

        struct HttpListenerImpl;
        HttpListenerImpl* impl_;
    };
}
