#include "ServiceEndpoints.h"
#include "ContextProviders.h"
#include "HttpStringHelpers.h"

using namespace Forum;
using namespace Forum::Commands;
using namespace Forum::Helpers;

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

static void updateContextForRequest(const Http::HttpRequest& request)
{
    Context::setCurrentUserIpAddress(request.remoteAddress);

    auto& displayContext = Context::getMutableDisplayContext();
    displayContext.sortOrder = Context::SortOrder::Ascending;
    displayContext.pageNumber = 0;
    displayContext.checkNotChangedSince = 0;

    for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
    {
        auto& name = request.queryPairs[i].first;
        auto& value = request.queryPairs[i].second;

        if (Http::matchStringUpperOrLower(name, "pPaAgGeE"))
        {
            Http::fromStringOrDefault(value, displayContext.pageNumber, 0);
        }
        else if (Http::matchStringUpperOrLower(name, "sSoOrRtT") && 
                 Http::matchStringUpperOrLower(value, "dDeEsScCeEnNdDiInNgG"))
        {
            displayContext.sortOrder = Context::SortOrder::Descending;
        }
    }
}


void AbstractEndpoint::handleDefault(Http::RequestState& requestState, View view, Command command,
                                     ExecuteCommandFn executeCommand)
{
    assert(nullptr != executeCommand);

    currentParameters.clear();
    updateContextForRequest(requestState.request);
    
    auto result = executeCommand(requestState, commandHandler_, view, command, currentParameters);

    requestState.response.writeResponseCode(requestState.request, commandStatusToHttpStatus(result.statusCode));
    requestState.response.writeHeader("Content-Type", "application/json");
    requestState.response.writeBodyAndContentLength(result.output.data(), result.output.size());
}

CommandHandler::Result AbstractEndpoint::defaultExecuteView(const Http::RequestState& requestState, 
                                                            CommandHandler& commandHandler, View view, Command command,
                                                            std::vector<StringView>& parameters)
{
    return commandHandler.handle(view, parameters);
}

MetricsEndpoint::MetricsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{   
}

void MetricsEndpoint::getVersion(Http::RequestState& requestState)
{
    handleDefault(requestState, View::SHOW_VERSION, {}, defaultExecuteView);
}

StatisticsEndpoint::StatisticsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void StatisticsEndpoint::getEntitiesCount(Http::RequestState& requestState)
{
    handleDefault(requestState, View::COUNT_ENTITIES, {}, defaultExecuteView);
}

UsersEndpoint::UsersEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

static const char OrderBy[] = "oOrRdDeErRbByY";
static const char OrderByCreated[] = "cCrReEaAtTeEdD";
static const char OrderByLastSeen[] = "lLaAsStTsSeEeEnN";
static const char OrderByLastUpdated[] = "lLaAsStTuUpPdDaAtTeEdD";
static const char OrderByThreadCount[] = "tThHrReEaAdDcCoOuUnNtT";
static const char OrderByMessageCount[] = "mMeEsSsSaAgGeEcCoOuUnNtT";

void UsersEndpoint::getAll(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_USERS_BY_NAME;
        auto& request = requestState.request;

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByCreated))
                {
                    view = View::GET_USERS_BY_CREATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByLastSeen))
                {
                    view = View::GET_USERS_BY_LAST_SEEN;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByThreadCount))
                {
                    view = View::GET_USERS_BY_THREAD_COUNT;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_USERS_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void UsersEndpoint::getUserById(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USER_BY_ID, parameters);
    });
}

void UsersEndpoint::getUserByName(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USER_BY_NAME, parameters);
    });
}

DiscussionThreadsEndpoint::DiscussionThreadsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionThreadsEndpoint::getAll(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_THREADS_BY_NAME;
        auto& request = requestState.request;

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_BY_CREATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByLastUpdated))
                {
                    view = View::GET_DISCUSSION_THREADS_BY_LAST_UPDATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void DiscussionThreadsEndpoint::getThreadById(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_THREAD_BY_ID, parameters);
    });
}

void DiscussionThreadsEndpoint::getThreadsOfUser(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_THREADS_OF_USER_BY_NAME;
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByLastUpdated))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void DiscussionThreadsEndpoint::getThreadsWithTag(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME;
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByLastUpdated))
                {
                    view = View::GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void DiscussionThreadsEndpoint::getThreadsOfCategory(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME;
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByLastUpdated))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void DiscussionThreadsEndpoint::getSubscribedThreadsOfUser(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_NAME;
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByCreated))
                {
                    view = View::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_CREATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByLastUpdated))
                {
                    view = View::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED;
                }
                else if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

DiscussionThreadMessagesEndpoint::DiscussionThreadMessagesEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionThreadMessagesEndpoint::getThreadMessagesOfUser(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getAllComments(Http::RequestState& requestState)
{
    return handleDefault(requestState, View::GET_MESSAGE_COMMENTS, {}, defaultExecuteView);
}

void DiscussionThreadMessagesEndpoint::getCommentsOfMessage(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getCommentsOfUser(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_MESSAGE_COMMENTS_OF_USER, parameters);
    });
}

DiscussionTagsEndpoint::DiscussionTagsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionTagsEndpoint::getAll(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_TAGS_BY_NAME;
        auto& request = requestState.request;

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

DiscussionCategoriesEndpoint::DiscussionCategoriesEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionCategoriesEndpoint::getAll(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {}, 
                  [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __, 
                     std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_CATEGORIES_BY_NAME;
        auto& request = requestState.request;

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void DiscussionCategoriesEndpoint::getRootCategories(Http::RequestState& requestState)
{
    return handleDefault(requestState, View::GET_DISCUSSION_CATEGORIES_FROM_ROOT, {}, defaultExecuteView);
}

void DiscussionCategoriesEndpoint::getCategoryById(Http::RequestState& requestState)
{
    handleDefault(requestState, {}, {},
        [](const Http::RequestState& requestState, CommandHandler& commandHandler, View _, Command __,
            std::vector<StringView>& parameters)
    {
        auto& request = requestState.request;
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_CATEGORY_BY_ID, parameters);
    });
}
