#include "CommandHandler.h"
#include "Configuration.h"
#include "OutputHelpers.h"

#include <cstddef>
#include <memory>

#include <boost/lexical_cast.hpp>

#include <unicode/ustring.h>
#include <unicode/unorm2.h>

using namespace Forum::Commands;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

#define COMMAND_HANDLER_METHOD(name) \
    StatusCode name(const std::vector<StringView>& parameters, OutStream& output)

static thread_local Json::StringBuffer outputBuffer{ 1 << 20 }; //1 MiByte buffer / thread initial and for each increment

static const std::string emptyString;

template<typename Collection>
auto countNonEmpty(const Collection& collection)
{
    return static_cast<size_t>(std::count_if(collection.begin(), collection.end(),
                               [](auto& view) { return view.size() > 0; }));
}

static constexpr size_t NormalizeBuffer16MaxChars = 2 << 20;
static constexpr size_t NormalizeBuffer8MaxChars = 2 * NormalizeBuffer16MaxChars;

static thread_local std::unique_ptr<UChar[]> normalizeBuffer16Before(new UChar[NormalizeBuffer16MaxChars]);
static thread_local std::unique_ptr<UChar[]> normalizeBuffer16After(new UChar[NormalizeBuffer16MaxChars]);
static thread_local std::unique_ptr<char[]> normalizeBuffer8(new char[NormalizeBuffer8MaxChars]);

/**
 * Performs a Unicode NFC normalization on a UTF-8 encoded string and returns a view also to a UTF-8 encoded string
 * If an error occurs or the input contains invalid characters, an empty view is returned
 */
static StringView normalize(StringView input)
{
    int32_t chars16Written = 0, chars8Written = 0;
    UErrorCode errorCode{};
    auto u8to16Result = u_strFromUTF8(normalizeBuffer16Before.get(), NormalizeBuffer16MaxChars, &chars16Written,
                                      input.data(), input.size(), &errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }

    errorCode = {};
    auto normalizer = unorm2_getNFCInstance(&errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }

    errorCode = {};
    auto chars16NormalizedWritten = unorm2_normalize(normalizer, u8to16Result, chars16Written,
                                                     normalizeBuffer16After.get(), NormalizeBuffer16MaxChars, &errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }

    errorCode = {};
    auto u16to8Result = u_strToUTF8(normalizeBuffer8.get(), NormalizeBuffer8MaxChars, &chars8Written,
                                    normalizeBuffer16After.get(), chars16NormalizedWritten, &errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }
    return StringView(u16to8Result, chars8Written);
}

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
    AuthorizationRepositoryRef authorizationRepository;
    StatisticsRepositoryRef statisticsRepository;
    MetricsRepositoryRef metricsRepository;

    static bool checkNumberOfParameters(const std::vector<StringView>& parameters, size_t number)
    {
        if (countNonEmpty(parameters) != number)
        {
            return false;
        }
        return true;
    }

    template<typename T>
    static bool convertTo(StringView value, T& result)
    {
        if ( ! boost::conversion::try_lexical_convert(value.data(), value.size(), result))
        {
            return false;
        }
        return true;
    }

    static bool checkMinNumberOfParameters(const std::vector<StringView>& parameters, size_t number)
    {
        if (countNonEmpty(parameters) < number)
        {
            return false;
        }
        return true;
    }

    template<typename PrivilegeType, typename PrivilegeStringsType>
    static bool parsePrivilege(StringView value, PrivilegeType& output, PrivilegeStringsType& strings)
    {
        int i = 0;
        for (auto str : strings)
        {
            if (str == value)
            {
                output = static_cast<PrivilegeType>(i);
                return true;
            }
            i += 1;
        }
        return false;
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
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).size() < 1) return INVALID_PARAMETERS;
        return userRepository->addNewUser(normalizedParam, parameters[1], output);
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
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->getUserById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).size() < 1) return INVALID_PARAMETERS;
        return userRepository->getUserByName(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return userRepository->changeUserName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_INFO )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return userRepository->changeUserInfo(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( DELETE_USER )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
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
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).size() < 1) return INVALID_PARAMETERS;
        return discussionThreadRepository->addNewDiscussionThread(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return discussionThreadRepository->changeDiscussionThreadName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        uint16_t newDisplayOrder{ 0 };
        if ( ! convertTo(parameters[1], newDisplayOrder)) return INVALID_PARAMETERS;
        return discussionThreadRepository->changeDiscussionThreadPinDisplayOrder(parameters[0], newDisplayOrder, output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->deleteDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( MERGE_DISCUSSION_THREADS )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionThreadRepository->mergeDiscussionThreads(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output,
                                                                      RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output,
                                                                      RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output,
                                                                      RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output,
                                                                      RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( SUBSCRIBE_TO_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->subscribeToDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( UNSUBSCRIBE_FROM_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->unsubscribeFromDiscussionThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->addNewDiscussionMessageInThread(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->deleteDiscussionMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT )
    {
        if ( ! checkMinNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        auto& changeReason = parameters.size() > 2 ? parameters[2] : emptyString;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->changeDiscussionThreadMessageContent(parameters[0], normalizedParam,
                                                                                       changeReason, output);
    }

    COMMAND_HANDLER_METHOD( MOVE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->moveDiscussionThreadMessage(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( UP_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->upVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( DOWN_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->downVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( RESET_VOTE_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->resetVoteDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getDiscussionThreadMessagesOfUserByCreated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->addCommentToDiscussionThreadMessage(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( GET_MESSAGE_COMMENTS )
    {
        return discussionThreadMessageRepository->getMessageComments(output);
    }

    COMMAND_HANDLER_METHOD( GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getMessageCommentsOfDiscussionThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_MESSAGE_COMMENTS_OF_USER )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getMessageCommentsOfUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( SET_MESSAGE_COMMENT_SOLVED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->setMessageCommentToSolved(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).size() < 1) return INVALID_PARAMETERS;
        return discussionTagRepository->addNewDiscussionTag(normalizedParam, output);
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
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return discussionTagRepository->changeDiscussionTagName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_UI_BLOB )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->changeDiscussionTagUiBlob(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionTagRepository->deleteDiscussionTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG_TO_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->addDiscussionTagToThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( REMOVE_DISCUSSION_TAG_FROM_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->removeDiscussionTagFromThread(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( MERGE_DISCUSSION_TAG_INTO_OTHER_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionTagRepository->mergeDiscussionTags(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_CATEGORY )
    {
        if ( ! checkMinNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        auto& parentId = parameters.size() > 1 ? parameters[1] : emptyString;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).size() < 1) return INVALID_PARAMETERS;
        return discussionCategoryRepository->addNewDiscussionCategory(normalizedParam, parentId, output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORY_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
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
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).size() < 1) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_DESCRIPTION )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryDescription(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_PARENT )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryParent(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        int_fast16_t newDisplayOrder{ 0 };
        if ( ! convertTo(parameters[1], newDisplayOrder)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->changeDiscussionCategoryDisplayOrder(parameters[0], newDisplayOrder, output);
    }

    COMMAND_HANDLER_METHOD( DELETE_DISCUSSION_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->deleteDiscussionCategory(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_TAG_TO_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->addDiscussionTagToCategory(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( REMOVE_DISCUSSION_TAG_FROM_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->removeDiscussionTagFromCategory(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output,
                                                                          RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output,
                                                                          RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output,
                                                                          RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output,
                                                                          RetrieveDiscussionThreadsBy::MessageCount);
    }


    COMMAND_HANDLER_METHOD( GET_REQUIRED_PRIVILEGES_FOR_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getRequiredPrivilegesForThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_ASSIGNED_PRIVILEGES_FOR_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getAssignedPrivilegesForThreadMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadMessagePrivilegeForThreadMessage(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( GET_REQUIRED_PRIVILEGES_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getRequiredPrivilegesForThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DEFAULT_PRIVILEGE_DURATIONS_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getDefaultPrivilegeDurationsForThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_ASSIGNED_PRIVILEGES_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getAssignedPrivilegesForThread(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageRequiredPrivilegeForThread(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadRequiredPrivilegeForThread(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadMessageDefaultPrivilegeDuration privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadMessageDefaultPrivilegeDurationStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadMessagePrivilegeForThread(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionThreadPrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionThreadPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadPrivilegeForThread(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( GET_REQUIRED_PRIVILEGES_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getRequiredPrivilegesForTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DEFAULT_PRIVILEGE_DURATIONS_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getDefaultPrivilegeDurationsForTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_ASSIGNED_PRIVILEGES_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getAssignedPrivilegesForTag(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageRequiredPrivilegeForTag(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadRequiredPrivilegeForTag(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionTagPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionTagPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionTagRequiredPrivilegeForTag(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionThreadMessageDefaultPrivilegeDuration privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadMessageDefaultPrivilegeDurationStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadMessagePrivilegeForTag(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionThreadPrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionThreadPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadPrivilegeForTag(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_TAG_PRIVILEGE_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionTagPrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionTagPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionTagPrivilegeForTag(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( GET_REQUIRED_PRIVILEGES_FOR_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getRequiredPrivilegesForCategory(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_ASSIGNED_PRIVILEGES_FOR_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getAssignedPrivilegesForCategory(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        DiscussionCategoryPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionCategoryPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionCategoryRequiredPrivilegeForCategory(
                parameters[0], privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE_FOR_CATEGORY )
    {
        if ( ! checkNumberOfParameters(parameters, 5)) return INVALID_PARAMETERS;

        DiscussionCategoryPrivilege privilege;
        if ( ! parsePrivilege(parameters[2], privilege, DiscussionCategoryPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[3], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[4], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionCategoryPrivilegeForCategory(
                parameters[0], parameters[1], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES )
    {
        return authorizationRepository->getForumWideCurrentUserPrivileges(output);
    }

    COMMAND_HANDLER_METHOD( GET_FORUM_WIDE_REQUIRED_PRIVILEGES )
    {
        return authorizationRepository->getForumWideRequiredPrivileges(output);
    }

    COMMAND_HANDLER_METHOD( GET_FORUM_WIDE_DEFAULT_PRIVILEGE_DURATIONS )
    {
        return authorizationRepository->getForumWideDefaultPrivilegeDurations(output);
    }

    COMMAND_HANDLER_METHOD( GET_FORUM_WIDE_ASSIGNED_PRIVILEGES )
    {
        return authorizationRepository->getForumWideAssignedPrivileges(output);
    }

    COMMAND_HANDLER_METHOD( GET_FORUM_WIDE_ASSIGNED_PRIVILEGES_FOR_USER )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getForumWideAssignedPrivilegesForUser(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[0], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageRequiredPrivilege(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        DiscussionThreadPrivilege privilege;
        if ( ! parsePrivilege(parameters[0], privilege, DiscussionThreadPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadRequiredPrivilege(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        DiscussionTagPrivilege privilege;
        if ( ! parsePrivilege(parameters[0], privilege, DiscussionTagPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionTagRequiredPrivilege(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        DiscussionCategoryPrivilege privilege;
        if ( ! parsePrivilege(parameters[0], privilege, DiscussionCategoryPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionCategoryRequiredPrivilege(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        ForumWidePrivilege privilege;
        if ( ! parsePrivilege(parameters[0], privilege, ForumWidePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeForumWideRequiredPrivilege(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        DiscussionThreadMessageDefaultPrivilegeDuration privilege;
        if ( ! parsePrivilege(parameters[0], privilege, DiscussionThreadMessageDefaultPrivilegeDurationStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeDiscussionThreadMessageDefaultPrivilegeDuration(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_DURATION )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        ForumWideDefaultPrivilegeDuration privilege;
        if ( ! parsePrivilege(parameters[0], privilege, ForumWideDefaultPrivilegeDurationStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        return authorizationRepository->changeForumWideDefaultPrivilegeDuration(privilege, value, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        DiscussionThreadMessagePrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadMessagePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadMessagePrivilege(
                parameters[0], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        DiscussionThreadPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionThreadPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadPrivilege(
                parameters[0], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_TAG_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        DiscussionTagPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionTagPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionTagPrivilege(
                parameters[0], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        DiscussionCategoryPrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, DiscussionCategoryPrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionCategoryPrivilege(
                parameters[0], privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_FORUM_WIDE_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        ForumWidePrivilege privilege;
        if ( ! parsePrivilege(parameters[1], privilege, ForumWidePrivilegeStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDefaultDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignForumWidePrivilege(
                parameters[0], privilege, value, duration, output);
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
    AuthorizationRepositoryRef authorizationRepository,
    StatisticsRepositoryRef statisticsRepository,
    MetricsRepositoryRef metricsRepository) : impl_(new CommandHandlerImpl)
{
    impl_->observerRepository = observerRepository;
    impl_->userRepository = userRepository;
    impl_->discussionThreadRepository = discussionThreadRepository;
    impl_->discussionThreadMessageRepository = discussionThreadMessageRepository;
    impl_->discussionTagRepository = discussionTagRepository;
    impl_->discussionCategoryRepository = discussionCategoryRepository;
    impl_->authorizationRepository = authorizationRepository;
    impl_->statisticsRepository = statisticsRepository;
    impl_->metricsRepository = metricsRepository;

    setCommandHandler(ADD_USER);
    setCommandHandler(CHANGE_USER_NAME);
    setCommandHandler(CHANGE_USER_INFO);
    setCommandHandler(DELETE_USER);

    setCommandHandler(ADD_DISCUSSION_THREAD);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_NAME);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER);
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

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_THREAD_MESSAGE);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FOR_THREAD);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_THREAD);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FOR_THREAD);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG);
    setCommandHandler(CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FOR_TAG);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_TAG);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FOR_TAG);
    setCommandHandler(ASSIGN_DISCUSSION_TAG_PRIVILEGE_FOR_TAG);

    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY);
    setCommandHandler(ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE_FOR_CATEGORY);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION);
    setCommandHandler(CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_DURATION);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_PRIVILEGE);
    setCommandHandler(ASSIGN_DISCUSSION_TAG_PRIVILEGE);
    setCommandHandler(ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE);
    setCommandHandler(ASSIGN_FORUM_WIDE_PRIVILEGE);


    setViewHandler(SHOW_VERSION);
    setViewHandler(COUNT_ENTITIES);

    setViewHandler(GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES);

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

    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_THREAD_MESSAGE);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_THREAD_MESSAGE);
    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_THREAD);
    setViewHandler(GET_DEFAULT_PRIVILEGE_DURATIONS_FOR_THREAD);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_THREAD);
    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_TAG);
    setViewHandler(GET_DEFAULT_PRIVILEGE_DURATIONS_FOR_TAG);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_TAG);
    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_CATEGORY);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_CATEGORY);
    setViewHandler(GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES);
    setViewHandler(GET_FORUM_WIDE_REQUIRED_PRIVILEGES);
    setViewHandler(GET_FORUM_WIDE_DEFAULT_PRIVILEGE_DURATIONS);
    setViewHandler(GET_FORUM_WIDE_ASSIGNED_PRIVILEGES);
    setViewHandler(GET_FORUM_WIDE_ASSIGNED_PRIVILEGES_FOR_USER);
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
    if (Context::disableCommands())
    {
        return { StatusCode::NOT_ALLOWED, {} };
    }

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
    auto outputView = outputBuffer.view();
    if (outputView.size() < 1)
    {
        writeStatusCode(outputBuffer, statusCode);
    }
    return { statusCode, outputBuffer.view() };
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
    auto outputView = outputBuffer.view();
    if (outputView.size() < 1)
    {
        writeStatusCode(outputBuffer, statusCode);
    }
    return{ statusCode, outputView };
}
