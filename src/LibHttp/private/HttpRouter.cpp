#include "HttpRouter.h"

using namespace Http;

#include <fstream>

void HttpRouter::forward(const HttpRequest& request, HttpResponseBuilder& response)
{
    response.writeResponseCode(request.versionMajor, request.versionMinor, HttpStatusCode::Not_Found);

    static const char reply[] = "No resource was found for the provided path.";

    response.writeBodyAndContentLength(reply, std::extent<decltype(reply)>::value - 1);
}
