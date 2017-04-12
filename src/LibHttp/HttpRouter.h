#pragma once

#include "HttpParser.h"
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
        StringView extraPathParts[MaxExtraPathParts];
        size_t nrOfExtraPathParts = 0;

    private:
        void extractExtraPathParts(size_t nrOfPathCharactersUsedInRoute);
    };

    class HttpRouter final : private boost::noncopyable
    {
    public:
        void forward(const HttpRequest& request, HttpResponseBuilder& response);

        typedef std::function<void(RequestState)> HandlerFn;

        constexpr static size_t MaxRouteSize = 128;

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

    private:

        struct StringViewByLengthGreater
        {
            constexpr bool operator()(StringView first, StringView second) const
            {
                return std::make_tuple(first.length(), first) > std::make_tuple(second.length(), second);
            }
        };

        std::map<StringView, HandlerFn, StringViewByLengthGreater> routes_[static_cast<size_t>(HttpVerb::HTTP_VERBS_COUNT)];
    };
}