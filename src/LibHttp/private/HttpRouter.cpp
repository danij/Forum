#include "HttpRouter.h"
#include "StringHelpers.h"

#include <algorithm>
#include <cstdint>

using namespace Http;

void RequestState::extractExtraPathParts(size_t nrOfPathCharactersUsedInRoute)
{
    //TODO
}

static void writeNotFound(const HttpRequest& request, HttpResponseBuilder& response)
{
    response.writeResponseCode(request.versionMajor, request.versionMinor, HttpStatusCode::Not_Found);

    static const char reply[] = "No resource was found for the provided path.";

    response.writeBodyAndContentLength(reply, std::extent<decltype(reply)>::value - 1);
}

static uint8_t getFirstIndexForRoutes(const char* path, size_t length)
{
    auto firstChar = length > 0 ? static_cast<uint8_t>(path[0]) : 0;
    return firstChar % HttpRouter::FirstIndexMaxValue;
}

void HttpRouter::forward(const HttpRequest& request, HttpResponseBuilder& response)
{
    char tempPath[MaxRouteSize + 1];
    auto tempPathLength = std::min(request.path.size(), MaxRouteSize);

    std::transform(request.path.begin(), request.path.begin() + tempPathLength, tempPath,
                   [](char c) { return static_cast<char>(CharToLower[c]); });

    if (tempPath[tempPathLength - 1] != '/')
    {
        tempPath[tempPathLength++] = '/';
    }
    
    bool routeFound = false;
    for (auto& pair : routes_[getFirstIndexForRoutes(tempPath, tempPathLength)][static_cast<size_t>(request.verb)])
    {
        auto& currentPath = pair.first;
        if (currentPath.size() <= tempPathLength)
        {
            if (std::equal(pair.first.begin(), pair.first.end(), tempPath))
            {
                if (pair.second)
                {
                    pair.second(Http::RequestState(request, response, currentPath.size()));
                }
                routeFound = true;
                break;
            }
        }
    }
    if ( ! routeFound)
    {
        writeNotFound(request, response);
    }
}

void HttpRouter::addRoute(StringView pathLowerCase, HttpVerb verb, HandlerFn&& handler)
{
    routes_[getFirstIndexForRoutes(pathLowerCase.data(), pathLowerCase.length())][static_cast<size_t>(verb)].emplace(pathLowerCase, handler);
}

