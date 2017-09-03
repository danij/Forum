#pragma once

#include "HttpRequest.h"
#include "HttpResponseBuilder.h"

#include <functional>
#include <map>
#include <tuple>

namespace Http
{
    struct RequestState
    {
        RequestState(const HttpRequest& request, HttpResponseBuilder& response, size_t nrOfPathCharactersUsedInRoute)
            : request(request), response(response)
        {
            extractExtraPathParts(nrOfPathCharactersUsedInRoute);
        }

        const HttpRequest& request;
        HttpResponseBuilder& response;

        static constexpr size_t MaxExtraPathParts = 32;
        HttpStringView extraPathParts[MaxExtraPathParts];
        size_t nrOfExtraPathParts = 0;

    private:
        void extractExtraPathParts(size_t nrOfPathCharactersUsedInRoute);
    };

    class HttpRouter final : private boost::noncopyable
    {
    public:
        void forward(const HttpRequest& request, HttpResponseBuilder& response);

        typedef std::function<void(RequestState&)> HandlerFn;

        static const size_t MaxRouteSize = 128;
        static const size_t FirstIndexMaxValue = 128;

        /**
         * Registers a route with a maximum size of MaxRouteSize.
         * Managing the lifetime of the string view and the handler is the responsibility of the caller
         *
         * @param pathLowerCase Lowercase version of the path to match, with trailing / but without leading /
         * @param verb Verb to match
         * @param handler Handler that will be called if the route is matched.
         *
         */
        void addRoute(HttpStringView pathLowerCase, HttpVerb verb, HandlerFn&& handler);

        /**
         * Registers a route to be used if no other route matches
         */
        void setDefaultRoute(HandlerFn&& handler);

    private:

        struct StringViewByLengthGreater
        {
            constexpr bool operator()(HttpStringView first, HttpStringView second) const
            {
                return std::make_tuple(first.length(), first) > std::make_tuple(second.length(), second);
            }
        };

        typedef std::map<HttpStringView, HandlerFn, StringViewByLengthGreater> MapType;
        MapType routes_[FirstIndexMaxValue][static_cast<size_t>(HttpVerb::HTTP_VERBS_COUNT)];

        HandlerFn defaultRoute_;
    };
}
