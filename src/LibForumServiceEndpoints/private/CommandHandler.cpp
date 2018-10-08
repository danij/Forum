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

#include "CommandHandler.h"
#include "Configuration.h"
#include "OutputHelpers.h"
#include "StringHelpers.h"

#include <cstddef>
#include <memory>

#include <boost/lexical_cast.hpp>
#include <boost/thread/tss.hpp>

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

#define COMMAND_HANDLER_METHOD_SIMPLE(name) \
    StatusCode name(const std::vector<StringView>& /*parameters*/, OutStream& output)

static Json::StringBuffer& getOutputBuffer()
{
    static boost::thread_specific_ptr<Json::StringBuffer> value;

    if ( ! value.get())
    {
        value.reset(new Json::StringBuffer{ 1 << 20 }); //1 MiByte buffer / thread initial and for each increment
    }
    return *value;
}

static const std::string EmptyString;

template<typename Collection>
auto countNonEmpty(const Collection& collection)
{
    return static_cast<size_t>(std::count_if(collection.begin(), collection.end(),
                               [](auto& view) { return view.size() > 0; }));
}

static constexpr size_t NormalizeBuffer16MaxChars = 2 << 20;
static constexpr size_t NormalizeBuffer8MaxChars = 2 * NormalizeBuffer16MaxChars;

/**
 * Performs a Unicode NFC normalization on a UTF-8 encoded string and returns a view also to a UTF-8 encoded string
 * If an error occurs or the input contains invalid characters, an empty view is returned
 */
static StringView normalize(StringView input)
{
    static boost::thread_specific_ptr<UChar> normalizeBuffer16BeforePtr;
    static boost::thread_specific_ptr<UChar> normalizeBuffer16AfterPtr;
    static boost::thread_specific_ptr<char> normalizeBuffer8Ptr;

    if (onlyASCII(input)) {

        //no normalization needed
        return input;
    }

    if ( ! normalizeBuffer16BeforePtr.get())
    {
        normalizeBuffer16BeforePtr.reset(new UChar[NormalizeBuffer16MaxChars]);
    }
    auto* normalizeBuffer16Before = normalizeBuffer16BeforePtr.get();

    if ( ! normalizeBuffer16AfterPtr.get())
    {
        normalizeBuffer16AfterPtr.reset(new UChar[NormalizeBuffer16MaxChars]);
    }
    auto* normalizeBuffer16After = normalizeBuffer16AfterPtr.get();

    if ( ! normalizeBuffer8Ptr.get())
    {
        normalizeBuffer8Ptr.reset(new char[NormalizeBuffer8MaxChars]);
    }
    auto* normalizeBuffer8 = normalizeBuffer8Ptr.get();

    if (input.empty())
    {
        return input;
    }

    int32_t chars16Written = 0, chars8Written = 0;
    UErrorCode errorCode{};
    const auto u8to16Result = u_strFromUTF8(normalizeBuffer16Before, NormalizeBuffer16MaxChars, &chars16Written,
                                            input.data(), static_cast<int32_t>(input.size()), &errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }

    errorCode = {};
    const auto normalizer = unorm2_getNFCInstance(&errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }

    errorCode = {};
    const auto chars16NormalizedWritten = unorm2_normalize(normalizer, u8to16Result, chars16Written,
                                                           normalizeBuffer16After, NormalizeBuffer16MaxChars, &errorCode);
    if (U_FAILURE(errorCode))
    {
        return{};
    }

    errorCode = {};
    const auto u16to8Result = u_strToUTF8(normalizeBuffer8, NormalizeBuffer8MaxChars, &chars8Written,
                                          normalizeBuffer16After, chars16NormalizedWritten, &errorCode);
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
    AttachmentRepositoryRef attachmentRepository;
    AuthorizationRepositoryRef authorizationRepository;
    StatisticsRepositoryRef statisticsRepository;
    MetricsRepositoryRef metricsRepository;

    static bool checkNumberOfParameters(const std::vector<StringView>& parameters, const size_t number)
    {
        return countNonEmpty(parameters) == number;
    }

    static bool checkNumberOfParametersAtLeast(const std::vector<StringView>& parameters, const size_t number)
    {
        return countNonEmpty(parameters) >= number;
    }

    template<typename T>
    static bool convertTo(StringView value, T& result)
    {
        return boost::conversion::try_lexical_convert(value.data(), value.size(), result);
    }

    static bool checkMinNumberOfParameters(const std::vector<StringView>& parameters, const size_t number)
    {
        return countNonEmpty(parameters) >= number;
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

    COMMAND_HANDLER_METHOD_SIMPLE( SHOW_VERSION )
    {
        return metricsRepository->getVersion(output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( COUNT_ENTITIES )
    {
        return statisticsRepository->getEntitiesCount(output);
    }

    COMMAND_HANDLER_METHOD( ADD_USER )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return userRepository->addNewUser(normalizedParam, Context::getCurrentUserAuth(), output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_CURRENT_USER )
    {
        return userRepository->getCurrentUser(output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_USERS_BY_NAME )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::Name);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_USERS_BY_CREATED )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::Created);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_USERS_BY_LAST_SEEN )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::LastSeen);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_USERS_BY_THREAD_COUNT )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::ThreadCount);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_USERS_BY_MESSAGE_COUNT )
    {
        return userRepository->getUsers(output, RetrieveUsersBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_USERS_ONLINE )
    {
        return userRepository->getUsersOnline(output);
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
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return userRepository->getUserByName(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( GET_MULTIPLE_USERS_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->getMultipleUsersById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_MULTIPLE_USERS_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->getMultipleUsersByName(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( SEARCH_USERS_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return userRepository->searchUsersByName(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_LOGO )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->getUserLogo(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_VOTE_HISTORY )
    {
        return userRepository->getUserVoteHistory(output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_QUOTED_HISTORY )
    {
        return userRepository->getUserQuotedHistory(output);
    }

    COMMAND_HANDLER_METHOD( GET_USER_RECEIVED_PRIVATE_MESSAGES )
    {
        return userRepository->getReceivedPrivateMessages(output);
    }
    
    COMMAND_HANDLER_METHOD( GET_USER_SENT_PRIVATE_MESSAGES )
    {
        return userRepository->getSentPrivateMessages(output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
        return userRepository->changeUserName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_INFO )
    {
        if ( ! checkNumberOfParametersAtLeast(parameters, 1)) return INVALID_PARAMETERS;
        const StringView normalizedParam = normalize(parameters[1]);
        return userRepository->changeUserInfo(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_TITLE )
    {
        if ( ! checkNumberOfParametersAtLeast(parameters, 1)) return INVALID_PARAMETERS;
        const StringView normalizedParam = normalize(parameters[1]);
        return userRepository->changeUserTitle(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_SIGNATURE )
    {
        if ( ! checkNumberOfParametersAtLeast(parameters, 1)) return INVALID_PARAMETERS;
        const StringView normalizedParam = normalize(parameters[1]);
        return userRepository->changeUserSignature(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_ATTACHMENT_QUOTA )
    {
        if ( ! checkNumberOfParametersAtLeast(parameters, 1)) return INVALID_PARAMETERS;
        uint64_t newQuota{ 0 };
        if ( ! convertTo(parameters[1], newQuota)) return INVALID_PARAMETERS;
        return userRepository->changeUserAttachmentQuota(parameters[0], newQuota, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_USER_LOGO )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return userRepository->changeUserLogo(parameters[0], parameters[1], output);
    }

    COMMAND_HANDLER_METHOD( DELETE_USER_LOGO )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->deleteUserLogo(parameters[0], output);
    }
    
    COMMAND_HANDLER_METHOD( DELETE_USER )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->deleteUser(parameters[0], output);
    }
    
    COMMAND_HANDLER_METHOD( SEND_PRIVATE_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;

        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;

        return userRepository->sendPrivateMessage(parameters[0], normalizedParam, output);
    }
    
    COMMAND_HANDLER_METHOD( DELETE_PRIVATE_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return userRepository->deletePrivateMessage(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_THREADS_BY_NAME )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::Name);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_THREADS_BY_CREATED )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::Created);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_THREADS_BY_LAST_UPDATED )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::LastUpdated);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_THREADS_BY_LATEST_MESSAGE_CREATED )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::LatestMessageCreated);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT )
    {
        return discussionThreadRepository->getDiscussionThreads(output, RetrieveDiscussionThreadsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( ADD_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return discussionThreadRepository->addNewDiscussionThread(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_MULTIPLE_DISCUSSION_THREADS_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getMultipleDiscussionThreadsById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( SEARCH_DISCUSSION_THREADS_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return discussionThreadRepository->searchDiscussionThreadsByName(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
        return discussionThreadRepository->changeDiscussionThreadName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        uint16_t newDisplayOrder{ 0 };
        if ( ! convertTo(parameters[1], newDisplayOrder)) return INVALID_PARAMETERS;
        return discussionThreadRepository->changeDiscussionThreadPinDisplayOrder(parameters[0], newDisplayOrder, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_APPROVAL )
    {
        if ( ! checkMinNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionThreadRepository->changeDiscussionThreadApproval(parameters[0],
                                                                          "true" == parameters[1], output);
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

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfUser(parameters[0], output,
                                                                      RetrieveDiscussionThreadsBy::LatestMessageCreated);
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

    COMMAND_HANDLER_METHOD( GET_USERS_SUBSCRIBED_TO_DISCUSSION_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getUsersSubscribedToDiscussionThread(parameters[0], output);
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

    COMMAND_HANDLER_METHOD( GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getSubscribedDiscussionThreadsOfUser(parameters[0], output,
                                                                                RetrieveDiscussionThreadsBy::LatestMessageCreated);
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
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
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
        const auto changeReason = parameters.size() > 2 ? parameters[2] : EmptyString;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->changeDiscussionThreadMessageContent(parameters[0], normalizedParam,
                                                                                       changeReason, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_THREAD_MESSAGE_APPROVAL )
    {
        if ( ! checkMinNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->changeDiscussionThreadMessageApproval(parameters[0], 
                                                                                        "true" == parameters[1], output);
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

    COMMAND_HANDLER_METHOD( GET_MULTIPLE_DISCUSSION_THREAD_MESSAGES_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getMultipleDiscussionThreadMessagesById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getDiscussionThreadMessagesOfUserByCreated(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_LATEST_DISCUSSION_THREAD_MESSAGES )
    {
        return discussionThreadMessageRepository->getLatestDiscussionThreadMessages(output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREAD_MESSAGE_RANK )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->getDiscussionThreadMessageRank(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
        return discussionThreadMessageRepository->addCommentToDiscussionThreadMessage(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_MESSAGE_COMMENTS )
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
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return discussionTagRepository->addNewDiscussionTag(normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_TAGS_BY_NAME )
    {
        return discussionTagRepository->getDiscussionTags(output, RetrieveDiscussionTagsBy::Name);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_TAGS_BY_THREAD_COUNT )
    {
        return discussionTagRepository->getDiscussionTags(output, RetrieveDiscussionTagsBy::ThreadCount);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT )
    {
        return discussionTagRepository->getDiscussionTags(output, RetrieveDiscussionTagsBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_TAG_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
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

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_WITH_TAG_BY_LATEST_MESSAGE_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsWithTag(parameters[0], output, RetrieveDiscussionThreadsBy::LatestMessageCreated);
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
        const auto parentId = parameters.size() > 1 ? parameters[1] : EmptyString;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        return discussionCategoryRepository->addNewDiscussionCategory(normalizedParam, parentId, output);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_CATEGORY_BY_ID )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionCategoryRepository->getDiscussionCategoryById(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_CATEGORIES_BY_NAME )
    {
        return discussionCategoryRepository->getDiscussionCategories(output, RetrieveDiscussionCategoriesBy::Name);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT )
    {
        return discussionCategoryRepository->getDiscussionCategories(output, RetrieveDiscussionCategoriesBy::MessageCount);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_DISCUSSION_CATEGORIES_FROM_ROOT )
    {
        return discussionCategoryRepository->getDiscussionCategoriesFromRoot(output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_DISCUSSION_CATEGORY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
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

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LATEST_MESSAGE_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output,
                                                                          RetrieveDiscussionThreadsBy::LatestMessageCreated);
    }

    COMMAND_HANDLER_METHOD( GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return discussionThreadRepository->getDiscussionThreadsOfCategory(parameters[0], output,
                                                                          RetrieveDiscussionThreadsBy::MessageCount);
    }
    
    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_BY_CREATED )
    {
        return attachmentRepository->getAttachments(RetrieveAttachmentsBy::Created, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_BY_NAME )
    {
        return attachmentRepository->getAttachments(RetrieveAttachmentsBy::Name, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_BY_SIZE )
    {
        return attachmentRepository->getAttachments(RetrieveAttachmentsBy::Size, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_BY_APPROVAL )
    {
        return attachmentRepository->getAttachments(RetrieveAttachmentsBy::Approval, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_OF_USER_BY_CREATED )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->getAttachmentsOfUser(parameters[0], RetrieveAttachmentsBy::Created, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_OF_USER_BY_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->getAttachmentsOfUser(parameters[0], RetrieveAttachmentsBy::Name, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_OF_USER_BY_SIZE )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->getAttachmentsOfUser(parameters[0], RetrieveAttachmentsBy::Size, output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENTS_OF_USER_BY_APPROVAL )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->getAttachmentsOfUser(parameters[0], RetrieveAttachmentsBy::Approval, output);
    }

    COMMAND_HANDLER_METHOD( CAN_GET_ATTACHMENT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->canGetAttachment(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( GET_ATTACHMENT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->getAttachment(parameters[0], output);
    }
    
    COMMAND_HANDLER_METHOD( ADD_ATTACHMENT )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[0])).empty()) return INVALID_PARAMETERS;
        uint64_t size{ 0 };
        if ( ! convertTo(parameters[1], size)) return INVALID_PARAMETERS;
        return attachmentRepository->addNewAttachment(normalizedParam, size, output);
    }

    COMMAND_HANDLER_METHOD( DELETE_ATTACHMENT )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return attachmentRepository->deleteAttachment(parameters[0], output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_ATTACHMENT_NAME )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        StringView normalizedParam;
        if ((normalizedParam = normalize(parameters[1])).empty()) return INVALID_PARAMETERS;
        return attachmentRepository->changeAttachmentName(parameters[0], normalizedParam, output);
    }

    COMMAND_HANDLER_METHOD( CHANGE_ATTACHMENT_APPROVAL )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return attachmentRepository->changeAttachmentApproval(parameters[0], "true" == parameters[1], output);
    }
    
    COMMAND_HANDLER_METHOD( ADD_ATTACHMENT_TO_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return attachmentRepository->addAttachmentToDiscussionThreadMessage(parameters[0], parameters[1], output);
    }
    
    COMMAND_HANDLER_METHOD( REMOVE_ATTACHMENT_FROM_DISCUSSION_THREAD_MESSAGE )
    {
        if ( ! checkNumberOfParameters(parameters, 2)) return INVALID_PARAMETERS;
        return attachmentRepository->removeAttachmentFromDiscussionThreadMessage(parameters[0], parameters[1], output);
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

    COMMAND_HANDLER_METHOD( GET_REQUIRED_PRIVILEGES_FOR_THREAD )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getRequiredPrivilegesForThread(parameters[0], output);
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

    COMMAND_HANDLER_METHOD( GET_REQUIRED_PRIVILEGES_FOR_TAG )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getRequiredPrivilegesForTag(parameters[0], output);
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

    COMMAND_HANDLER_METHOD_SIMPLE( GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES )
    {
        return authorizationRepository->getForumWideCurrentUserPrivileges(output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_FORUM_WIDE_REQUIRED_PRIVILEGES )
    {
        return authorizationRepository->getForumWideRequiredPrivileges(output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVELS )
    {
        return authorizationRepository->getForumWideDefaultPrivilegeLevels(output);
    }

    COMMAND_HANDLER_METHOD_SIMPLE( GET_FORUM_WIDE_ASSIGNED_PRIVILEGES )
    {
        return authorizationRepository->getForumWideAssignedPrivileges(output);
    }

    COMMAND_HANDLER_METHOD( GET_ASSIGNED_PRIVILEGES_FOR_USER )
    {
        if ( ! checkNumberOfParameters(parameters, 1)) return INVALID_PARAMETERS;
        return authorizationRepository->getAssignedPrivilegesForUser(parameters[0], output);
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

    COMMAND_HANDLER_METHOD( CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVEL )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        ForumWideDefaultPrivilegeDuration privilege;
        if ( ! parsePrivilege(parameters[0], privilege, ForumWideDefaultPrivilegeDurationStrings)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        PrivilegeDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[2], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->changeForumWideDefaultPrivilegeLevel(privilege, value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadMessagePrivilege(
                parameters[0], parameters[1], value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_THREAD_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionThreadPrivilege(
                parameters[0], parameters[1], value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_TAG_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionTagPrivilege(
                parameters[0], parameters[1], value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 4)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[2], value)) return INVALID_PARAMETERS;

        PrivilegeDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[3], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignDiscussionCategoryPrivilege(
                parameters[0], parameters[1], value, duration, output);
    }

    COMMAND_HANDLER_METHOD( ASSIGN_FORUM_WIDE_PRIVILEGE )
    {
        if ( ! checkNumberOfParameters(parameters, 3)) return INVALID_PARAMETERS;

        PrivilegeValueIntType value{ 0 };
        if ( ! convertTo(parameters[1], value)) return INVALID_PARAMETERS;

        PrivilegeDurationIntType duration{ 0 };
        if ( ! convertTo(parameters[2], duration)) return INVALID_PARAMETERS;

        return authorizationRepository->assignForumWidePrivilege(parameters[0], value, duration, output);
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
    AttachmentRepositoryRef attachmentRepository,
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
    impl_->attachmentRepository = attachmentRepository;
    impl_->authorizationRepository = authorizationRepository;
    impl_->statisticsRepository = statisticsRepository;
    impl_->metricsRepository = metricsRepository;

    setCommandHandler(ADD_USER);
    setCommandHandler(CHANGE_USER_NAME);
    setCommandHandler(CHANGE_USER_INFO);
    setCommandHandler(CHANGE_USER_TITLE);
    setCommandHandler(CHANGE_USER_SIGNATURE);
    setCommandHandler(CHANGE_USER_ATTACHMENT_QUOTA);
    setCommandHandler(CHANGE_USER_LOGO);
    setCommandHandler(DELETE_USER_LOGO);
    setCommandHandler(DELETE_USER);
    setCommandHandler(SEND_PRIVATE_MESSAGE);
    setCommandHandler(DELETE_PRIVATE_MESSAGE);

    setCommandHandler(ADD_DISCUSSION_THREAD);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_NAME);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_APPROVAL);
    setCommandHandler(DELETE_DISCUSSION_THREAD);
    setCommandHandler(MERGE_DISCUSSION_THREADS);

    setCommandHandler(ADD_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(DELETE_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_APPROVAL);
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
    
    setCommandHandler(ADD_ATTACHMENT);
    setCommandHandler(DELETE_ATTACHMENT);
    setCommandHandler(CHANGE_ATTACHMENT_NAME);
    setCommandHandler(CHANGE_ATTACHMENT_APPROVAL);
    setCommandHandler(ADD_ATTACHMENT_TO_DISCUSSION_THREAD_MESSAGE);
    setCommandHandler(REMOVE_ATTACHMENT_FROM_DISCUSSION_THREAD_MESSAGE);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG);
    setCommandHandler(CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG);

    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY);

    setCommandHandler(CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE);
    setCommandHandler(CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVEL);

    setCommandHandler(ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE);
    setCommandHandler(ASSIGN_DISCUSSION_THREAD_PRIVILEGE);
    setCommandHandler(ASSIGN_DISCUSSION_TAG_PRIVILEGE);
    setCommandHandler(ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE);
    setCommandHandler(ASSIGN_FORUM_WIDE_PRIVILEGE);


    setViewHandler(SHOW_VERSION);
    setViewHandler(COUNT_ENTITIES);

    setViewHandler(GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES);

    setViewHandler(GET_CURRENT_USER);
    setViewHandler(GET_USERS_BY_NAME);
    setViewHandler(GET_USERS_BY_CREATED);
    setViewHandler(GET_USERS_BY_LAST_SEEN);
    setViewHandler(GET_USERS_BY_THREAD_COUNT);
    setViewHandler(GET_USERS_BY_MESSAGE_COUNT);
    setViewHandler(GET_USERS_ONLINE);
    setViewHandler(GET_USER_BY_ID);
    setViewHandler(GET_USER_BY_NAME);
    setViewHandler(GET_MULTIPLE_USERS_BY_ID);
    setViewHandler(GET_MULTIPLE_USERS_BY_NAME);
    setViewHandler(SEARCH_USERS_BY_NAME);
    setViewHandler(GET_USER_LOGO);
    setViewHandler(GET_USER_VOTE_HISTORY);
    setViewHandler(GET_USER_QUOTED_HISTORY);
    setViewHandler(GET_USER_RECEIVED_PRIVATE_MESSAGES);
    setViewHandler(GET_USER_SENT_PRIVATE_MESSAGES);

    setViewHandler(GET_DISCUSSION_THREADS_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_BY_LATEST_MESSAGE_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
    setViewHandler(GET_DISCUSSION_THREAD_BY_ID);
    setViewHandler(GET_MULTIPLE_DISCUSSION_THREADS_BY_ID);
    setViewHandler(SEARCH_DISCUSSION_THREADS_BY_NAME);

    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT);

    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_NAME);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_CREATED);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED);
    setViewHandler(GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT);
    setViewHandler(GET_USERS_SUBSCRIBED_TO_DISCUSSION_THREAD);

    setViewHandler(GET_MULTIPLE_DISCUSSION_THREAD_MESSAGES_BY_ID);
    setViewHandler(GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED);
    setViewHandler(GET_LATEST_DISCUSSION_THREAD_MESSAGES);
    setViewHandler(GET_DISCUSSION_THREAD_MESSAGE_RANK);

    setViewHandler(GET_MESSAGE_COMMENTS);
    setViewHandler(GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE);
    setViewHandler(GET_MESSAGE_COMMENTS_OF_USER);

    setViewHandler(GET_DISCUSSION_TAGS_BY_NAME);
    setViewHandler(GET_DISCUSSION_TAGS_BY_THREAD_COUNT);
    setViewHandler(GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT);

    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_LATEST_MESSAGE_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT);

    setViewHandler(GET_DISCUSSION_CATEGORY_BY_ID);
    setViewHandler(GET_DISCUSSION_CATEGORIES_BY_NAME);
    setViewHandler(GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT);
    setViewHandler(GET_DISCUSSION_CATEGORIES_FROM_ROOT);

    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LATEST_MESSAGE_CREATED);
    setViewHandler(GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT);

    setViewHandler(GET_ATTACHMENTS_BY_CREATED);
    setViewHandler(GET_ATTACHMENTS_BY_NAME);
    setViewHandler(GET_ATTACHMENTS_BY_SIZE);
    setViewHandler(GET_ATTACHMENTS_BY_APPROVAL);
    setViewHandler(GET_ATTACHMENTS_OF_USER_BY_CREATED);
    setViewHandler(GET_ATTACHMENTS_OF_USER_BY_NAME);
    setViewHandler(GET_ATTACHMENTS_OF_USER_BY_SIZE);
    setViewHandler(GET_ATTACHMENTS_OF_USER_BY_APPROVAL);
    setViewHandler(CAN_GET_ATTACHMENT);
    setViewHandler(GET_ATTACHMENT);

    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_THREAD_MESSAGE);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_THREAD_MESSAGE);
    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_THREAD);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_THREAD);
    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_TAG);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_TAG);
    setViewHandler(GET_REQUIRED_PRIVILEGES_FOR_CATEGORY);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_CATEGORY);
    setViewHandler(GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES);
    setViewHandler(GET_FORUM_WIDE_REQUIRED_PRIVILEGES);
    setViewHandler(GET_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVELS);
    setViewHandler(GET_FORUM_WIDE_ASSIGNED_PRIVILEGES);
    setViewHandler(GET_ASSIGNED_PRIVILEGES_FOR_USER);
}

CommandHandler::~CommandHandler()
{
    delete impl_;
}

ReadEvents& CommandHandler::readEvents()
{
    return impl_->observerRepository->readEvents();
}

WriteEvents& CommandHandler::writeEvents()
{
    return impl_->observerRepository->writeEvents();
}

CommandHandler::Result CommandHandler::handle(const Command command, const std::vector<StringView>& parameters)
{
    impl_->userRepository->updateCurrentUserId();

    const auto config = getGlobalConfig();

    if (config->service.disableCommands)
    {
        return{ StatusCode::NOT_ALLOWED, {} };
    }

    if (config->service.disableCommandsForAnonymousUsers && ( ! Context::getCurrentUserId()))
    {
        return{ StatusCode::NOT_ALLOWED, {} };
    }

    auto& outputBuffer = getOutputBuffer();

    outputBuffer.clear();

    StatusCode statusCode;
    if (command >= 0 && command < LAST_COMMAND)
    {
        statusCode = impl_->commandHandlers[command](parameters, outputBuffer);
    }
    else
    {
        statusCode = StatusCode::NOT_FOUND;
    }
    auto outputView = outputBuffer.view();
    if (outputView.empty())
    {
        writeStatusCode(outputBuffer, statusCode);
        outputView = outputBuffer.view();
    }
    return { statusCode, outputBuffer.view() };
}

CommandHandler::Result CommandHandler::handle(const View view, const std::vector<StringView>& parameters)
{
    impl_->userRepository->updateCurrentUserId();
    
    auto& outputBuffer = getOutputBuffer();

    outputBuffer.clear();

    StatusCode statusCode;
    if (view >= 0 && view < LAST_VIEW)
    {
        statusCode = impl_->viewHandlers[view](parameters, outputBuffer);
    }
    else
    {
        statusCode = StatusCode::NOT_FOUND;
    }
    auto outputView = outputBuffer.view();
    if (outputView.empty())
    {
        writeStatusCode(outputBuffer, statusCode);
        outputView = outputBuffer.view();
    }
    return{ statusCode, outputView };
}
