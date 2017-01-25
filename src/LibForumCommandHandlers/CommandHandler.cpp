#include "CommandHandler.h"

#include "OutputHelpers.h"

#include <memory>

using namespace Forum::Commands;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

#define COMMAND_HANDLER_METHOD(name) \
    void name(const std::vector<std::string>& parameters, std::ostream& output)

struct Forum::Commands::CommandHandler::CommandHandlerImpl
{
    std::function<void(const std::vector<std::string>&, std::ostream&)> handlers[int(LAST_COMMAND)];
    ReadRepositoryRef readRepository;
    WriteRepositoryRef writeRepository;
    MetricsRepositoryRef metricsRepository;

    bool static checkNumberOfParameters(const std::vector<std::string>& parameters, std::ostream& output, size_t number)
    {
        if (parameters.size() != number)
        {
            writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
            return false;
        }
        return true;
    }

    COMMAND_HANDLER_METHOD( SHOW_VERSION )
    {
        metricsRepository->getVersion(output);
    }

    COMMAND_HANDLER_METHOD( COUNT_ENTITIES )
    {
        readRepository->getEntitiesCount(output);
    }

    COMMAND_HANDLER_METHOD( ADD_USER )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->addNewUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_NAME )
    {
        readRepository->getUsersByName(output);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_CREATED )
    {
        readRepository->getUsersByCreated(output);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_LAST_SEEN )
    {
        readRepository->getUsersByLastSeen(output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getUserById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getUserByName(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->changeUserName(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_USER )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->deleteUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_NAME )
    {
        readRepository->getDiscussionThreadsByName(output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_CREATED )
    {
        readRepository->getDiscussionThreadsByCreated(output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_LAST_UPDATED )
    {
        readRepository->getDiscussionThreadsByLastUpdated(output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT )
    {
        readRepository->getDiscussionThreadsByMessageCount(output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->addNewDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->changeDiscussionThreadName(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->deleteDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( MERGE_DISCUSSION_THREADS )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->mergeDiscussionThreads(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsOfUserByName(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsOfUserByCreated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsOfUserByLastUpdated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsOfUserByMessageCount(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->addNewDiscussionMessageInThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->deleteDiscussionMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->changeDiscussionThreadMessageContent(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( MOVE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->moveDiscussionThreadMessage(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( UP_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->upVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( DOWN_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->downVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( RESET_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->resetVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadMessagesOfUserByCreated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->addNewDiscussionTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_TAGS_BY_NAME )
    {
        readRepository->getDiscussionTagsByName(output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT )
    {
        readRepository->getDiscussionTagsByMessageCount(output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->changeDiscussionTagName(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_UI_BLOB )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->changeDiscussionTagUiBlob(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        writeRepository->deleteDiscussionTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsWithTagByName(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsWithTagByCreated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsWithTagByLastUpdated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return;
        readRepository->getDiscussionThreadsWithTagByMessageCount(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG_TO_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->addDiscussionTagToThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( REMOVE_DISCUSSION_TAG_FROM_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->removeDiscussionTagFromThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( MERGE_DISCUSSION_TAG_INTO_OTHER_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return;
        writeRepository->mergeDiscussionTags(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_CATEGORY )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORY_BY_ID )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORIES_BY_NAME )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORIES_FROM_ROOT )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_NAME )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_DESCRIPTION )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_PARENT )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_CATEGORY )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG_TO_CATEGORY )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( REMOVE_DISCUSSION_TAG_FROM_CATEGORY )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED )
    {
        output << "{}";
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT )
    {
        output << "{}";
    }
};


#define setHandler(command) \
    impl_->handlers[command] = [&](auto& parameters, auto& output) { impl_->command(parameters, output); }

CommandHandler::CommandHandler(ReadRepositoryRef readRepository, WriteRepositoryRef writeRepository,
    MetricsRepositoryRef metricsRepository) : impl_(new CommandHandlerImpl)
{
    impl_->readRepository = readRepository;
    impl_->writeRepository = writeRepository;
    impl_->metricsRepository = metricsRepository;

    setHandler(SHOW_VERSION);
    setHandler(COUNT_ENTITIES);

    setHandler(ADD_USER);
    setHandler(GET_USERS_BY_NAME);
    setHandler(GET_USERS_BY_CREATED);
    setHandler(GET_USERS_BY_LAST_SEEN);
    setHandler(GET_USER_BY_ID);
    setHandler(GET_USER_BY_NAME);
    setHandler(CHANGE_USER_NAME);
    setHandler(DELETE_USER);

    setHandler(ADD_DISCUSSION_THREAD);
    setHandler(GET_DISCUSSION_THREADS_BY_NAME);
    setHandler(GET_DISCUSSION_THREADS_BY_CREATED);
    setHandler(GET_DISCUSSION_THREADS_BY_LAST_UPDATED);
    setHandler(GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
    setHandler(GET_DISCUSSION_THREAD_BY_ID);
    setHandler(CHANGE_DISCUSSION_THREAD_NAME);
    setHandler(DELETE_DISCUSSION_THREAD);
    setHandler(MERGE_DISCUSSION_THREADS);

    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_NAME);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_CREATED);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED);
    setHandler(GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT);

    setHandler(ADD_DISCUSSION_THREAD_MESSAGE);
    setHandler(DELETE_DISCUSSION_THREAD_MESSAGE);
    setHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT);
    setHandler(MOVE_DISCUSSION_THREAD_MESSAGE);
    setHandler(UP_VOTE_DISCUSSION_THREAD_MESSAGE);
    setHandler(DOWN_VOTE_DISCUSSION_THREAD_MESSAGE);
    setHandler(RESET_VOTE_DISCUSSION_THREAD_MESSAGE);

    setHandler(GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED);

    setHandler(ADD_DISCUSSION_TAG);
    setHandler(GET_DISCUSSION_TAGS_BY_NAME);
    setHandler(GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT);
    setHandler(CHANGE_DISCUSSION_TAG_NAME);
    setHandler(CHANGE_DISCUSSION_TAG_UI_BLOB);
    setHandler(DELETE_DISCUSSION_TAG);
    setHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME);
    setHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED);
    setHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED);
    setHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT);
    setHandler(ADD_DISCUSSION_TAG_TO_THREAD);
    setHandler(REMOVE_DISCUSSION_TAG_FROM_THREAD);
    setHandler(MERGE_DISCUSSION_TAG_INTO_OTHER_TAG);

    setHandler(ADD_DISCUSSION_CATEGORY);
    setHandler(GET_DISCUSSION_CATEGORY_BY_ID);
    setHandler(GET_DISCUSSION_CATEGORIES_BY_NAME);
    setHandler(GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT);
    setHandler(GET_DISCUSSION_CATEGORIES_FROM_ROOT);
    setHandler(CHANGE_DISCUSSION_CATEGORY_NAME);
    setHandler(CHANGE_DISCUSSION_CATEGORY_DESCRIPTION);
    setHandler(CHANGE_DISCUSSION_CATEGORY_PARENT);
    setHandler(CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER);
    setHandler(DELETE_DISCUSSION_CATEGORY);
    setHandler(ADD_DISCUSSION_TAG_TO_CATEGORY);
    setHandler(REMOVE_DISCUSSION_TAG_FROM_CATEGORY);
    setHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME);
    setHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED);
    setHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED);
    setHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT);
}

CommandHandler::~CommandHandler()
{
    if (impl_)
    {
        delete impl_;
    }
}

ReadRepositoryRef CommandHandler::getReadRepository()
{
    return impl_->readRepository;
}

WriteRepositoryRef CommandHandler::getWriteRepository()
{
    return impl_->writeRepository;
}

void CommandHandler::handle(Command command, const std::vector<std::string>& parameters, std::ostream& output)
{
    if (command >= 0 && command < LAST_COMMAND)
    {
        impl_->handlers[command](parameters, output);
    }
}
