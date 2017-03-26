#include "CommandHandler.h"
#include "Configuration.h"
#include "OutputHelpers.h"

#include <memory>

#include <boost/lexical_cast.hpp>

using namespace Forum::Commands;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

#define COMMAND_HANDLER_METHOD(name) \
    StatusCode name(const std::vector<StringView>& parameters, OutStream& output)

static thread_local std::string outputBuffer;

static const std::string emptyString;

struct CommandHandler::CommandHandlerImpl
{
    std::function<StatusCode(const std::vector<StringView>&, OutStream&)> commandHandlers[int(LAST_COMMAND)];
    std::function<StatusCode(const std::vector<StringView>&, OutStream&)> viewHandlers[int(LAST_VIEW)];

    ObservableRepositoryRef observerRepository;
    UserRepositoryRef userRepository;
    DiscussionThreadRepositoryRef discussionThreadRepository;
    DiscussionThreadMessageRepositoryRef discussionThreadMessageRepository;
    DiscussionTagRepositoryRef discussionTagRepository;
    DiscussionCategoryRepositoryRef discussionCategoryRepository;
    StatisticsRepositoryRef statisticsRepository;
    MetricsRepositoryRef metricsRepository;

    CommandHandlerImpl()
    {
        outputBuffer.reserve(getGlobalConfig()->service.serializationPerThreadBufferSize);
    }

    static bool checkNumberOfParameters(const std::vector<StringView>& parameters, OutStream& output, size_t number)
    {
        if (parameters.size() != number)
        {
            writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
            return false;
        }
        return true;
    }

    template<typename T>
    static bool convertTo(const StringView& value, T& result, OutStream& output)
    {
        try
        {
            result = boost::lexical_cast<T>(value.data(), value.size());
            return true;
        }
        catch (...)
        {
            writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
            return false;
        }
    }

    static bool checkMinNumberOfParameters(const std::vector<StringView>& parameters, OutStream& output, size_t number)
    {
        if (parameters.size() < number)
        {
            writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
            return false;
        }
        return true;
    }

    COMMAND_HANDLER_METHOD( SHOW_VERSION )
    {
        return metricsRepository->getVersion(output);
    }

    COMMAND_HANDLER_METHOD( COUNT_ENTITIES )
    {
        return statisticsRepository->getEntitiesCount(output);
    }

    COMMAND_HANDLER_METHOD( ADD_USER )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return userRepository->addNewUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_NAME )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_CREATED )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_LAST_SEEN )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::LastSeen);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_THREAD_COUNT )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::ThreadCount);
    }

    COMMAND_HANDLER_METHOD( GET_USERS_BY_MESSAGE_COUNT )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( GET_USER_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return userRepository->getUserById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return userRepository->getUserByName(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return userRepository->changeUserName(parameters[0], parameters[1], output);
    }
    
    COMMAND_HANDLER_METHOD( CHANGE_USER_INFO )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return userRepository->changeUserInfo(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_USER )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return userRepository->deleteUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_NAME )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_CREATED )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_LAST_UPDATED )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->addNewDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionThreadRepository->changeDiscussionThreadName(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->deleteDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( MERGE_DISCUSSION_THREADS )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionThreadRepository->mergeDiscussionThreads(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output, 
                                                                      RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output, 
                                                                      RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output, 
                                                                      RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output, 
                                                                      RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( SUBSCRIBE_TO_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->subscribeToDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( UNSUBSCRIBE_FROM_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->unsubscribeFromDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output, 
                                                                                RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::MessageCount);
    }
    
    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->addNewDiscussionMessageInThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->deleteDiscussionMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT )
    {
        if ( ! checkMinNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        auto& changeReason = parameters.size() > 2 ? parameters[2] : emptyString;
        return discussionThreadMessageRepository->changeDiscussionThreadMessageContent(parameters[0], parameters[1], 
                                                                                       changeReason, output);
    }

    COMMAND_HANDLER_METHOD( MOVE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->moveDiscussionThreadMessage(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( UP_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->upVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( DOWN_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->downVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( RESET_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->resetVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getDiscussionThreadMessagesOfUserByCreated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->addCommentToDiscussionThreadMessage(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( GET_MESSAGE_COMMENTS )
    {
        return discussionThreadMessageRepository->getMessageComments(output);
    }

    COMMAND_HANDLER_METHOD( GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getMessageCommentsOfDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_MESSAGE_COMMENTS_OF_USER )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getMessageCommentsOfUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( SET_MESSAGE_COMMENT_SOLVED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->setMessageCommentToSolved(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionTagRepository->addNewDiscussionTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_TAGS_BY_NAME )
    {
        return discussionTagRepository->getDiscussionTags(output, RetrieveDiscussionTagsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT )
    {
        return discussionTagRepository->getDiscussionTags(output, RetrieveDiscussionTagsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->changeDiscussionTagName(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_UI_BLOB )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->changeDiscussionTagUiBlob(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionTagRepository->deleteDiscussionTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG_TO_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->addDiscussionTagToThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( REMOVE_DISCUSSION_TAG_FROM_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->removeDiscussionTagFromThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( MERGE_DISCUSSION_TAG_INTO_OTHER_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->mergeDiscussionTags(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_CATEGORY )
    {
        if ( ! checkMinNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        auto& parentId = parameters.size() > 1 ? parameters[1] : emptyString;
        return discussionCategoryRepository->addNewDiscussionCategory(parameters[0], parentId, output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORY_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->getDiscussionCategoryById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORIES_BY_NAME )
    {
        return discussionCategoryRepository->getDiscussionCategories(output, RetrieveDiscussionCategoriesBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT )
    {
        return discussionCategoryRepository->getDiscussionCategories(output, RetrieveDiscussionCategoriesBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORIES_FROM_ROOT )
    {
        return discussionCategoryRepository->getDiscussionCategoriesFromRoot(output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryName(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_DESCRIPTION )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryDescription(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_PARENT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryParent(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        int_fast16_t newDisplayOrder{ 0 };
        if ( ! convertTo(parameters[1], newDisplayOrder, output)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryDisplayOrder(parameters[0], newDisplayOrder, output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->deleteDiscussionCategory(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG_TO_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->addDiscussionTagToCategory(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( REMOVE_DISCUSSION_TAG_FROM_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, output, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->removeDiscussionTagFromCategory(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output, 
                                                                          RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output, 
                                                                          RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output, 
                                                                          RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, output, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output, 
                                                                          RetrieveDiscussionThreadsBy::MessageCount);
    }
};


#define setCommandHandler(command) \
    impl_->commandHandlers[command] = [&](auto& parameters, auto& output) { return impl_->command(parameters, output); }

#define setViewHandler(command) \
    impl_->viewHandlers[command] = [&](auto& parameters, auto& output) { return impl_->command(parameters, output); }

CommandHandler::CommandHandler(ObservableRepositoryRef observerRepository,
    UserRepositoryRef userRepository,
    DiscussionThreadRepositoryRef discussionThreadRepository,
    DiscussionThreadMessageRepositoryRef discussionThreadMessageRepository,
    DiscussionTagRepositoryRef discussionTagRepository,
    DiscussionCategoryRepositoryRef discussionCategoryRepository,
    StatisticsRepositoryRef statisticsRepository,
    MetricsRepositoryRef metricsRepository) : impl_(new CommandHandlerImpl)
{
    impl_->observerRepository = observerRepository;
    impl_->userRepository = userRepository;
    impl_->discussionThreadRepository = discussionThreadRepository;
    impl_->discussionThreadMessageRepository = discussionThreadMessageRepository;
    impl_->discussionTagRepository = discussionTagRepository;
    impl_->discussionCategoryRepository = discussionCategoryRepository;
    impl_->statisticsRepository = statisticsRepository;
    impl_->metricsRepository = metricsRepository;

    setCommandHandler(ADD_USER);
    setCommandHandler(CHANGE_USER_NAME);
    setCommandHandler(CHANGE_USER_INFO);
    setCommandHandler(DELETE_USER);

    setCommandHandler(ADD_DISCUSSION_THREAD);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_NAME);
    setCommandHandler(DELETE_DISCUSSION_THREAD);
    setCommandHandler(MERGE_DISCUSSION_THREADS);

    setCommandHandler(ADD_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(DELETE_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT);
    setCommandHandler(MOVE_DISCUSSION_THREAD_MESSAGE);

    setCommandHandler(UP_VOTE_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(DOWN_VOTE_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(RESET_VOTE_DISCUSSION_THREAD_MESSAGE);

    setCommandHandler(SUBSCRIBE_TO_THREAD);
    setCommandHandler(UNSUBSCRIBE_FROM_THREAD);

    setCommandHandler(ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(SET_MESSAGE_COMMENT_SOLVED);

    setCommandHandler(ADD_DISCUSSION_TAG);
    setCommandHandler(CHANGE_DISCUSSION_TAG_NAME);
    setCommandHandler(CHANGE_DISCUSSION_TAG_UI_BLOB);
    setCommandHandler(DELETE_DISCUSSION_TAG);
    setCommandHandler(ADD_DISCUSSION_TAG_TO_THREAD);
    setCommandHandler(REMOVE_DISCUSSION_TAG_FROM_THREAD);
    setCommandHandler(MERGE_DISCUSSION_TAG_INTO_OTHER_TAG);

    setCommandHandler(ADD_DISCUSSION_CATEGORY);
    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_NAME);
    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_DESCRIPTION);
    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_PARENT);
    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER);
    setCommandHandler(DELETE_DISCUSSION_CATEGORY);
    setCommandHandler(ADD_DISCUSSION_TAG_TO_CATEGORY);
    setCommandHandler(REMOVE_DISCUSSION_TAG_FROM_CATEGORY);

    setViewHandler(SHOW_VERSION);
    setViewHandler(COUNT_ENTITIES);

    setViewHandler(GET_USERS_BY_NAME);
    setViewHandler(GET_USERS_BY_CREATED);
    setViewHandler(GET_USERS_BY_LAST_SEEN);
    setViewHandler(GET_USERS_BY_THREAD_COUNT);
    setViewHandler(GET_USERS_BY_MESSAGE_COUNT);
    setViewHandler(GET_USER_BY_ID);
    setViewHandler(GET_USER_BY_NAME);

    setViewHandler(GET_DISCUSSION_THREADS_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
    setViewHandler(GET_DISCUSSION_THREAD_BY_ID);

    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT);

    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_NAME);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_CREATED);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT);

    setViewHandler(GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED);

    setViewHandler(GET_MESSAGE_COMMENTS);
    setViewHandler(GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE);
    setViewHandler(GET_MESSAGE_COMMENTS_OF_USER);

    setViewHandler(GET_DISCUSSION_TAGS_BY_NAME);
    setViewHandler(GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT);

    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT);

    setViewHandler(GET_DISCUSSION_CATEGORY_BY_ID);
    setViewHandler(GET_DISCUSSION_CATEGORIES_BY_NAME);
    setViewHandler(GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT);
    setViewHandler(GET_DISCUSSION_CATEGORIES_FROM_ROOT);

    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT);
}

CommandHandler::~CommandHandler()
{
    if (impl_)
    {
        delete impl_;
    }
}

ReadEvents& CommandHandler::readEvents()
{
    return impl_->observerRepository->readEvents();
}

WriteEvents& CommandHandler::writeEvents()
{
    return impl_->observerRepository->writeEvents();
}

CommandHandler::Result CommandHandler::handle(Command command, const std::vector<StringView>& parameters)
{
    StatusCode statusCode;
    if (command >= 0 && command < LAST_COMMAND)
    {
        outputBuffer.clear();
        statusCode = impl_->commandHandlers[command](parameters, outputBuffer);
    }
    else
    {
        statusCode = StatusCode::NOT_FOUND;
    }
    return { statusCode, outputBuffer };
}

CommandHandler::Result CommandHandler::handle(View view, const std::vector<StringView>& parameters)
{
    StatusCode statusCode;
    if (view >= 0 && view < LAST_VIEW)
    {
        outputBuffer.clear();
        statusCode = impl_->viewHandlers[view](parameters, outputBuffer);
    }
    else
    {
        statusCode = StatusCode::NOT_FOUND;
    }
    return{ statusCode, outputBuffer };
}
