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

#include "ServiceEndpoints.h"
#include "Configuration.h"
#include "ContextProviders.h"
#include "HttpStringHelpers.h"

#include <boost/thread/tss.hpp>

#include <vector>

using namespace Forum;
using namespace Forum::Commands;
using namespace Forum::Helpers;

AbstractEndpoint::AbstractEndpoint(CommandHandler& handler) : commandHandler_(handler)
{
    const auto config = Configuration::getGlobalConfig();
    prefix_ = config->service.responsePrefix;
}

Http::HttpStatusCode commandStatusToHttpStatus(const Repository::StatusCode code)
{
    switch (code)
    {
    //case Repository::OK: break;
    //case Repository::INVALID_PARAMETERS: break;
    //case Repository::VALUE_TOO_LONG: break;
    //case Repository::VALUE_TOO_SHORT: break;
    //case Repository::ALREADY_EXISTS: break;
    //case Repository::USER_WITH_SAME_AUTH_ALREADY_EXISTS: break;
    //case Repository::NO_EFFECT: break;
    case Repository::NOT_FOUND:
        return Http::HttpStatusCode::Not_Found;
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

        if (Http::matchStringUpperOrLower(name, "pagePAGE"))
        {
            Http::fromStringOrDefault(value, displayContext.pageNumber, static_cast<decltype(displayContext.pageNumber)>(0));
        }
        else if (Http::matchStringUpperOrLower(name, "sortSORT") &&
                 Http::matchStringUpperOrLower(value, "descendingDESCENDING"))
        {
            displayContext.sortOrder = Context::SortOrder::Descending;
        }
    }
}

static StringView getPointerToEntireRequestBody(const Http::HttpRequest& request)
{
    static boost::thread_specific_ptr<std::vector<char>> currentRequestContentPtr;

    if ( ! currentRequestContentPtr.get())
    {
        currentRequestContentPtr.reset(new std::vector<char>(Http::Buffer::MaxRequestBodyLength));
    }
    auto& currentRequestContent = *currentRequestContentPtr;

    if (request.nrOfRequestContentBuffers < 1)
    {
        return{};
    }
    if (request.nrOfRequestContentBuffers < 2)
    {
        return request.requestContentBuffers[0];
    }

    currentRequestContent.clear();

    for (size_t i = 0; i < request.nrOfRequestContentBuffers; ++i)
    {
        auto& buffer = request.requestContentBuffers [i];
        currentRequestContent.insert(currentRequestContent.end(), buffer.begin(), buffer.end());
    }

    return StringView(currentRequestContent.data(), currentRequestContent.size());
}

void AbstractEndpoint::handle(Http::RequestState& requestState, ExecuteFn executeCommand)
{
    handleInternal(requestState, "application/json", executeCommand, true);
}

void AbstractEndpoint::handleBinary(Http::RequestState& requestState, const StringView contentType, 
                                    const ExecuteFn executeCommand)
{
    handleInternal(requestState, contentType, executeCommand, false);
}

void AbstractEndpoint::handleInternal(Http::RequestState& requestState, const StringView contentType,
                                      const ExecuteFn executeCommand, const bool writePrefix)
{
    assert(nullptr != executeCommand);

    currentParameters.clear();
    updateContextForRequest(requestState.request);

    const auto result = executeCommand(requestState, commandHandler_, currentParameters);

    auto& response = requestState.response;

    response.writeResponseCode(requestState.request, commandStatusToHttpStatus(result.statusCode));
    response.writeHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    if (result.statusCode == Repository::StatusCode::OK)
    {
        response.writeHeader("Content-Type", contentType);
    }
    else
    {
        response.writeHeader("Content-Type", "application/json");
    }
    if (writePrefix)
    {
        response.writeBodyAndContentLength(result.output, prefix_);
    }
    else
    {
        response.writeBodyAndContentLength(result.output);
    }
}

MetricsEndpoint::MetricsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void MetricsEndpoint::getVersion(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::SHOW_VERSION, parameters);
    });
}

StatisticsEndpoint::StatisticsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void StatisticsEndpoint::getEntitiesCount(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::COUNT_ENTITIES, parameters);
    });
}

UsersEndpoint::UsersEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

static const char OrderBy[] = "orderbyORDERBY";
static const char OrderByCreated[] = "createdCREATED";
static const char OrderByLastSeen[] = "lastseenLASTSEEN";
static const char OrderByLastUpdated[] = "lastupdatedLASTUPDATED";
static const char OrderByLatestMessageCreated[] = "latestmessagecreatedLATESTMESSAGECREATED";
static const char OrderByThreadCount[] = "threadcountTHREADCOUNT";
static const char OrderByMessageCount[] = "messagecountMESSAGECOUNT";

void UsersEndpoint::getAll(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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

void UsersEndpoint::getOnline(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_USERS_ONLINE, parameters);
    });
}

void UsersEndpoint::getUserById(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USER_BY_ID, parameters);
    });
}

void UsersEndpoint::getUserByName(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USER_BY_NAME, parameters);
    });
}

void UsersEndpoint::searchUsersByName(Http::RequestState& requestState)
{
    handle(requestState,
        [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::SEARCH_USERS_BY_NAME, parameters);
    });
}

void UsersEndpoint::getUserLogo(Http::RequestState& requestState)
{
    handleBinary(requestState, "image/png",
                 [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USER_LOGO, parameters);
    });
}

void UsersEndpoint::getUserVoteHistory(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USER_VOTE_HISTORY, parameters);
    });
}

void UsersEndpoint::getUsersSubscribedToThread(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_USERS_SUBSCRIBED_TO_DISCUSSION_THREAD, parameters);
    });
}

void UsersEndpoint::add(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::ADD_USER, parameters);
    });
}

void UsersEndpoint::remove(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DELETE_USER, parameters);
    });
}

void UsersEndpoint::changeName(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_USER_NAME, parameters);
    });
}

void UsersEndpoint::changeInfo(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_USER_INFO, parameters);
    });
}

void UsersEndpoint::changeTitle(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_USER_TITLE, parameters);
    });
}

void UsersEndpoint::changeSignature(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_USER_SIGNATURE, parameters);
    });
}

void UsersEndpoint::changeLogo(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_USER_LOGO, parameters);
    });
}

void UsersEndpoint::deleteLogo(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DELETE_USER_LOGO, parameters);
    });
}


DiscussionThreadsEndpoint::DiscussionThreadsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionThreadsEndpoint::getAll(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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
                else if (Http::matchStringUpperOrLower(value, OrderByLatestMessageCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_BY_LATEST_MESSAGE_CREATED;
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
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_THREAD_BY_ID, parameters);
    });
}

void DiscussionThreadsEndpoint::getMultipleThreadsById(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_MULTIPLE_DISCUSSION_THREADS_BY_ID, parameters);
    });
}

void DiscussionThreadsEndpoint::getThreadsOfUser(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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
                else if (Http::matchStringUpperOrLower(value, OrderByLatestMessageCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED;
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
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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
                else if (Http::matchStringUpperOrLower(value, OrderByLatestMessageCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_WITH_TAG_BY_LATEST_MESSAGE_CREATED;
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
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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
                else if (Http::matchStringUpperOrLower(value, OrderByLatestMessageCreated))
                {
                    view = View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LATEST_MESSAGE_CREATED;
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

void DiscussionThreadsEndpoint::searchThreadsByName(Http::RequestState& requestState)
{
    handle(requestState,
        [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::SEARCH_DISCUSSION_THREADS_BY_NAME, parameters);
    });
}

void DiscussionThreadsEndpoint::getSubscribedThreadsOfUser(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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
                else if (Http::matchStringUpperOrLower(value, OrderByLatestMessageCreated))
                {
                    view = View::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED;
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

void DiscussionThreadsEndpoint::add(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::ADD_DISCUSSION_THREAD, parameters);
    });
}

void DiscussionThreadsEndpoint::remove(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DELETE_DISCUSSION_THREAD, parameters);
    });
}

void DiscussionThreadsEndpoint::changeName(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_NAME, parameters);
    });
}

void DiscussionThreadsEndpoint::changePinDisplayOrder(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER, parameters);
    });
}

void DiscussionThreadsEndpoint::merge(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::MERGE_DISCUSSION_THREADS, parameters);
    });
}

void DiscussionThreadsEndpoint::subscribe(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::SUBSCRIBE_TO_THREAD, parameters);
    });
}

void DiscussionThreadsEndpoint::unsubscribe(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::UNSUBSCRIBE_FROM_THREAD, parameters);
    });
}

void DiscussionThreadsEndpoint::addTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::ADD_DISCUSSION_TAG_TO_THREAD, parameters);
    });
}

void DiscussionThreadsEndpoint::removeTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::REMOVE_DISCUSSION_TAG_FROM_THREAD, parameters);
    });
}

DiscussionThreadMessagesEndpoint::DiscussionThreadMessagesEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionThreadMessagesEndpoint::getMultipleThreadMessagesById(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_MULTIPLE_DISCUSSION_THREAD_MESSAGES_BY_ID, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getThreadMessagesOfUser(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getLatestThreadMessages(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_LATEST_DISCUSSION_THREAD_MESSAGES, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getRankOfMessage(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_THREAD_MESSAGE_RANK, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getAllComments(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_MESSAGE_COMMENTS, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getCommentsOfMessage(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::getCommentsOfUser(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_MESSAGE_COMMENTS_OF_USER, parameters);
    });
}
void DiscussionThreadMessagesEndpoint::add(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::ADD_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::remove(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DELETE_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::changeContent(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::move(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::MOVE_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::upVote(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::UP_VOTE_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::downVote(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::resetVote(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::RESET_VOTE_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::addComment(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE, parameters);
    });
}

void DiscussionThreadMessagesEndpoint::setCommentSolved(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::SET_MESSAGE_COMMENT_SOLVED, parameters);
    });
}

DiscussionTagsEndpoint::DiscussionTagsEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionTagsEndpoint::getAll(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        auto view = View::GET_DISCUSSION_TAGS_BY_NAME;
        auto& request = requestState.request;

        for (size_t i = 0; i < request.nrOfQueryPairs; ++i)
        {
            auto& name = request.queryPairs[i].first;
            auto& value = request.queryPairs[i].second;

            if (Http::matchStringUpperOrLower(name, OrderBy))
            {
                if (Http::matchStringUpperOrLower(value, OrderByThreadCount))
                {
                    view = View::GET_DISCUSSION_TAGS_BY_THREAD_COUNT;
                }
                if (Http::matchStringUpperOrLower(value, OrderByMessageCount))
                {
                    view = View::GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT;
                }
            }
        }
        return commandHandler.handle(view, parameters);
    });
}

void DiscussionTagsEndpoint::add(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::ADD_DISCUSSION_TAG, parameters);
    });
}

void DiscussionTagsEndpoint::remove(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DELETE_DISCUSSION_TAG, parameters);
    });
}

void DiscussionTagsEndpoint::changeName(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_TAG_NAME, parameters);
    });
}

void DiscussionTagsEndpoint::changeUiBlob(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_TAG_UI_BLOB, parameters);
    });
}

void DiscussionTagsEndpoint::merge(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG, parameters);
    });
}

DiscussionCategoriesEndpoint::DiscussionCategoriesEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void DiscussionCategoriesEndpoint::getAll(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
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
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_DISCUSSION_CATEGORIES_FROM_ROOT, parameters);
    });
}

void DiscussionCategoriesEndpoint::getCategoryById(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_DISCUSSION_CATEGORY_BY_ID, parameters);
    });
}

void DiscussionCategoriesEndpoint::add(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::ADD_DISCUSSION_CATEGORY, parameters);
    });
}

void DiscussionCategoriesEndpoint::remove(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::DELETE_DISCUSSION_CATEGORY, parameters);
    });
}

void DiscussionCategoriesEndpoint::changeName(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_CATEGORY_NAME, parameters);
    });
}

void DiscussionCategoriesEndpoint::changeDescription(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_CATEGORY_DESCRIPTION, parameters);
    });
}

void DiscussionCategoriesEndpoint::changeParent(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_CATEGORY_PARENT, parameters);
    });
}

void DiscussionCategoriesEndpoint::changeDisplayOrder(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(getPointerToEntireRequestBody(requestState.request));
        return commandHandler.handle(Command::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER, parameters);
    });
}

void DiscussionCategoriesEndpoint::addTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::ADD_DISCUSSION_TAG_TO_CATEGORY, parameters);
    });
}

void DiscussionCategoriesEndpoint::removeTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(Command::REMOVE_DISCUSSION_TAG_FROM_CATEGORY, parameters);
    });
}


AuthorizationEndpoint::AuthorizationEndpoint(CommandHandler& handler) : AbstractEndpoint(handler)
{
}

void AuthorizationEndpoint::getRequiredPrivilegesForThreadMessage(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_REQUIRED_PRIVILEGES_FOR_THREAD_MESSAGE, parameters);
    });
}

void AuthorizationEndpoint::getAssignedPrivilegesForThreadMessage(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_ASSIGNED_PRIVILEGES_FOR_THREAD_MESSAGE, parameters);
    });
}

void AuthorizationEndpoint::getRequiredPrivilegesForThread(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_REQUIRED_PRIVILEGES_FOR_THREAD, parameters);
    });
}

void AuthorizationEndpoint::getAssignedPrivilegesForThread(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_ASSIGNED_PRIVILEGES_FOR_THREAD, parameters);
    });
}

void AuthorizationEndpoint::getRequiredPrivilegesForTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_REQUIRED_PRIVILEGES_FOR_TAG, parameters);
    });
}

void AuthorizationEndpoint::getAssignedPrivilegesForTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_ASSIGNED_PRIVILEGES_FOR_TAG, parameters);
    });
}

void AuthorizationEndpoint::getRequiredPrivilegesForCategory(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_REQUIRED_PRIVILEGES_FOR_CATEGORY, parameters);
    });
}

void AuthorizationEndpoint::getAssignedPrivilegesForCategory(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_ASSIGNED_PRIVILEGES_FOR_CATEGORY, parameters);
    });
}

void AuthorizationEndpoint::getForumWideCurrentUserPrivileges(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES, parameters);
    });
}

void AuthorizationEndpoint::getForumWideRequiredPrivileges(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_FORUM_WIDE_REQUIRED_PRIVILEGES, parameters);
    });
}

void AuthorizationEndpoint::getForumWideDefaultPrivilegeLevels(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVELS, parameters);
    });
}

void AuthorizationEndpoint::getForumWideAssignedPrivileges(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& /*requestState*/, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        return commandHandler.handle(View::GET_FORUM_WIDE_ASSIGNED_PRIVILEGES, parameters);
    });
}

void AuthorizationEndpoint::getAssignedPrivilegesForUser(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        return commandHandler.handle(View::GET_ASSIGNED_PRIVILEGES_FOR_USER, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadMessageRequiredPrivilegeForThread(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadRequiredPrivilegeForThread(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadMessageRequiredPrivilegeForTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadRequiredPrivilegeForTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionTagRequiredPrivilegeForTag(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionCategoryRequiredPrivilegeForCategory(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadMessageRequiredPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionThreadRequiredPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionTagRequiredPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::changeDiscussionCategoryRequiredPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::changeForumWideRequiredPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        return commandHandler.handle(Command::CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::changeForumWideDefaultPrivilegeLevel(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVEL, parameters);
    });
}

void AuthorizationEndpoint::assignDiscussionThreadMessagePrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        parameters.push_back(requestState.extraPathParts[3]);
        return commandHandler.handle(Command::ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::assignDiscussionThreadPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        parameters.push_back(requestState.extraPathParts[3]);
        return commandHandler.handle(Command::ASSIGN_DISCUSSION_THREAD_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::assignDiscussionTagPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        parameters.push_back(requestState.extraPathParts[3]);
        return commandHandler.handle(Command::ASSIGN_DISCUSSION_TAG_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::assignDiscussionCategoryPrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        parameters.push_back(requestState.extraPathParts[3]);
        return commandHandler.handle(Command::ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE, parameters);
    });
}

void AuthorizationEndpoint::assignForumWidePrivilege(Http::RequestState& requestState)
{
    handle(requestState,
           [](const Http::RequestState& requestState, CommandHandler& commandHandler, std::vector<StringView>& parameters)
    {
        parameters.push_back(requestState.extraPathParts[0]);
        parameters.push_back(requestState.extraPathParts[1]);
        parameters.push_back(requestState.extraPathParts[2]);
        return commandHandler.handle(Command::ASSIGN_FORUM_WIDE_PRIVILEGE, parameters);
    });
}
