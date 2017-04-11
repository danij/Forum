#pragma once

#include "HttpParser.h"
#include "HttpResponseBuilder.h"

namespace Http
{
    struct RequestState
    {
        RequestState(const HttpRequest& request, HttpResponseBuilder& response) 
            : request(request), response(response)
        {            
        }

        const HttpRequest& request;
        HttpResponseBuilder& response;
    };

    class HttpRouter final : private boost::noncopyable
    {
    public:
        void forward(const HttpRequest& request, HttpResponseBuilder& response);
    };
}