/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "HttpRouter.h"
#include "HttpStringHelpers.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>

using namespace Http;

const size_t HttpRouter::MaxRouteSize;
const size_t HttpRouter::FirstIndexMaxValue;


void RequestState::extractExtraPathParts(size_t nrOfPathCharactersUsedInRoute)
{
    auto path = request.path.data();
    if (nrOfPathCharactersUsedInRoute > 0 && path[nrOfPathCharactersUsedInRoute - 1] != '/'
        && request.path.size() > nrOfPathCharactersUsedInRoute)
    {
        nrOfPathCharactersUsedInRoute += 1;
    }
    auto currentPartStartIndex = nrOfPathCharactersUsedInRoute;

    for (auto i = nrOfPathCharactersUsedInRoute, n = request.path.size(); i < n; ++i)
    {
        if (nrOfExtraPathParts >= (MaxExtraPathParts - 1))
        {
            return;
        }
        if (path[i] == '/')
        {
            extraPathParts[nrOfExtraPathParts++] = HttpStringView(path + currentPartStartIndex, i - currentPartStartIndex);
            currentPartStartIndex = i + 1;
        }
    }
    auto lastPartLength = request.path.size() - currentPartStartIndex;
    if ((lastPartLength > 0) && (lastPartLength <= request.path.size()))
    {
        extraPathParts[nrOfExtraPathParts++] = HttpStringView(path + currentPartStartIndex, lastPartLength);
    }
}

static void writeNotFound(const HttpRequest& request, HttpResponseBuilder& response)
{
    response.writeResponseCode(request, HttpStatusCode::Not_Found);

    static const char reply[] = "No resource was found for the provided path.";

    response.writeBodyAndContentLength(HttpStringView(reply, std::extent<decltype(reply)>::value - 1));
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

    static_assert(std::extent<decltype(CharToLower)>::value > 255, "CharToLower is not big enough");

    std::transform(request.path.begin(), request.path.begin() + tempPathLength, tempPath,
                   [](char c) { return static_cast<char>(CharToLower[static_cast<uint8_t>(c)]); });

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
                    RequestState state(request, response, currentPath.size());
                    pair.second(state);
                }
                routeFound = true;
                break;
            }
        }
    }
    if ( ! routeFound)
    {
        if (defaultRoute_)
        {
            RequestState state(request, response, 0);
            defaultRoute_(state);
        }
        else
        {
            writeNotFound(request, response);
        }
    }
}

void HttpRouter::addRoute(HttpStringView pathLowerCase, HttpVerb verb, HandlerFn&& handler)
{
    routes_[getFirstIndexForRoutes(pathLowerCase.data(), pathLowerCase.length())][static_cast<size_t>(verb)].emplace(pathLowerCase, handler);
}

void HttpRouter::setDefaultRoute(HandlerFn&& handler)
{
    defaultRoute_ = std::move(handler);
}
