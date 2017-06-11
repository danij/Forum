#pragma once

#include "CommandHandler.h"
#include "HttpRouter.h"

#include <vector>

namespace Forum
{
    namespace Commands
    {
        //Endpoints can be called from multiple threads

        class AbstractEndpoint
        {
        public:
            explicit AbstractEndpoint(CommandHandler& handler);

        protected:
            typedef CommandHandler::Result(*ExecuteCommandFn)(const Http::RequestState&, CommandHandler&, View, Command,
                                                              std::vector<StringView>&);

            void handleDefault(Http::RequestState& requestState, View view, Command command,
                               ExecuteCommandFn executeCommand);

            static CommandHandler::Result defaultExecuteView(const Http::RequestState&, CommandHandler&, View, Command,
                                                             std::vector<StringView>&);

            CommandHandler& commandHandler_;
        };

        class MetricsEndpoint : private AbstractEndpoint
        {
        public:
            explicit MetricsEndpoint(CommandHandler& handler);

            void getVersion(Http::RequestState& requestState);
        };

        class StatisticsEndpoint : private AbstractEndpoint
        {
        public:
            explicit StatisticsEndpoint(CommandHandler& handler);

            void getEntitiesCount(Http::RequestState& requestState);
        };

        class UsersEndpoint : private AbstractEndpoint
        {
        public:
            explicit UsersEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getUserById(Http::RequestState& requestState);
            void getUserByName(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeInfo(Http::RequestState& requestState);
        };

        class DiscussionThreadsEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionThreadsEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getThreadById(Http::RequestState& requestState);
            void getThreadsOfUser(Http::RequestState& requestState);
            void getThreadsWithTag(Http::RequestState& requestState);
            void getThreadsOfCategory(Http::RequestState& requestState);
            void getSubscribedThreadsOfUser(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changePinDisplayOrder(Http::RequestState& requestState);
            void merge(Http::RequestState& requestState);
            void subscribe(Http::RequestState& requestState);
            void unsubscribe(Http::RequestState& requestState);
            void addTag(Http::RequestState& requestState);
            void removeTag(Http::RequestState& requestState);
        };

        class DiscussionThreadMessagesEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionThreadMessagesEndpoint(CommandHandler& handler);

            void getThreadMessagesOfUser(Http::RequestState& requestState);
            void getAllComments(Http::RequestState& requestState);
            void getCommentsOfMessage(Http::RequestState& requestState);
            void getCommentsOfUser(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeContent(Http::RequestState& requestState);
            void move(Http::RequestState& requestState);
            void upVote(Http::RequestState& requestState);
            void downVote(Http::RequestState& requestState);
            void resetVote(Http::RequestState& requestState);
            void addComment(Http::RequestState& requestState);
            void setCommentSolved(Http::RequestState& requestState);
        };

        class DiscussionTagsEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionTagsEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeUiBlob(Http::RequestState& requestState);
            void merge(Http::RequestState& requestState);
        };

        class DiscussionCategoriesEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionCategoriesEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getRootCategories(Http::RequestState& requestState);
            void getCategoryById(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeDescription(Http::RequestState& requestState);
            void changeParent(Http::RequestState& requestState);
            void changeDisplayOrder(Http::RequestState& requestState);
            void addTag(Http::RequestState& requestState);
            void removeTag(Http::RequestState& requestState);
        };
    }
}
