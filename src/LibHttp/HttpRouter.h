#pragma once

#include "HttpParser.h"
#include "HttpResponseBuilder.h"

#include <functional>
#include <map>
#include <tuple>

#include <boost/asio/ip/address.hpp>

#ifdef DELETE
#undef DELETE
#endif

namespace Http
{
    struct RequestState
    {
        RequestState(const HttpRequest& request, HttpResponseBuilder& response, size_t nrOfPathCharactersUsedInRoute,
                     const boost::asio::ip::address& remoteAddress)
            : request(request), response(response), remoteAddress(remoteAddress)
        {
            extractExtraPathParts(nrOfPathCharactersUsedInRoute);
        }

        const HttpRequest& request;
        HttpResponseBuilder& response;
        boost::asio::ip::address remoteAddress;

        static constexpr size_t MaxExtraPathParts = 32;
        StringView extraPathParts[MaxExtraPathParts];
        size_t nrOfExtraPathParts = 0;

    private:
        void extractExtraPathParts(size_t nrOfPathCharactersUsedInRoute);
    };

    class HttpRouter final : private boost::noncopyable
    {
    public:
        void forward(const HttpRequest& request, HttpResponseBuilder& response,
                     const boost::asio::ip::address& remoteAddress);

        typedef std::function<void(RequestState&)> HandlerFn;

        static constexpr size_t MaxRouteSize = 128;
        static constexpr size_t FirstIndexMaxValue = 128;

        /**
         * Registers a route with a maximum size of MaxRouteSize.
         * Managing the lifetime of the string view and the handler is the responsibility of the caller
         *
         * @param pathLowerCase Lowercase version of the path to match, with trailing / but without leading /
         * @param verb Verb to match
         * @param handler Handler that will be called if the route is matched.
         * 
         */
        void addRoute(StringView pathLowerCase, HttpVerb verb, HandlerFn&& handler);

        /**
         * Registers a route to be used if no other route matches
         */
        void setDefaultRoute(HandlerFn&& handler);

    private:

        struct StringViewByLengthGreater
        {
            constexpr bool operator()(StringView first, StringView second) const
            {
                return std::make_tuple(first.length(), first) > std::make_tuple(second.length(), second);
            }
        };

        typedef std::map<StringView, HandlerFn, StringViewByLengthGreater> MapType;
        MapType routes_[FirstIndexMaxValue][static_cast<size_t>(HttpVerb::HTTP_VERBS_COUNT)];

        HandlerFn defaultRoute_;
    };
}