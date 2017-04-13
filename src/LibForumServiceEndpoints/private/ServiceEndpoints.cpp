#include "ServiceEndpoints.h"

using namespace Forum;
using namespace Forum::Commands;

AbstractEndpoint::AbstractEndpoint(CommandHandler& handler) : commandHandler_(handler)
{
}

Http::HttpStatusCode commandStatusToHttpStatus(Repository::StatusCode code)
{
    switch (code)
    {
    //case Repository::OK: break;
    //case Repository::INVALID_PARAMETERS: break;
    //case Repository::VALUE_TOO_LONG: break;
    //case Repository::VALUE_TOO_SHORT: break;
    //case Repository::ALREADY_EXISTS: break;
    //case Repository::NOT_FOUND: break;
    //case Repository::NO_EFFECT: break;
    case Repository::CIRCULAR_REFERENCE_NOT_ALLOWED: 
        return Http::HttpStatusCode::Forbidden;
    case Repository::NOT_ALLOWED: 
        return Http::HttpStatusCode::Forbidden;
    case Repository::NOT_UPDATED_SINCE_LAST_CHECK: 
        return Http::HttpStatusCode::Not_Modified;
    case Repository::UNAUTHORIZED: 
        return Http::HttpStatusCode::Unauthorized;
    case Repository::THROTTLED: 
        return Http::HttpStatusCode::Too_Many_Requests;
    default: 
        return Http::HttpStatusCode::OK;
    }
}

//reserve space for the parameter views up-front so that no reallocations should occur when handling invididual requests
static thread_local std::vector<StringView> currentParameters{ 128 };

void AbstractEndpoint::handleDefault(Http::RequestState& requestState, View view)
{
    currentParameters.clear();
    auto result = commandHandler_.handle(view, currentParameters);

    requestState.response.writeResponseCode(requestState.request, commandStatusToHttpStatus(result.statusCode));
    requestState.response.writeHeader("Content-Type", "application/json");
    requestState.response.writeBodyAndContentLength(result.output.data(), result.output.size());
}

MetricsEndpoint::MetricsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void MetricsEndpoint::getVersion(Http::RequestState& requestState)
{
    handleDefault(requestState, View::SHOW_VERSION);
}

StatisticsEndpoint::StatisticsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void StatisticsEndpoint::getEntitiesCount(Http::RequestState& requestState)
{
    handleDefault(requestState, View::COUNT_ENTITIES);
}
