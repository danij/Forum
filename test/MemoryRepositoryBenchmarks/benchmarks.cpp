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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <cstdlib>

#include <unicode/uclean.h>

#include "DefaultAuthorization.h"
#include "MemoryRepositoryCommon.h"
#include "MemoryRepositoryUser.h"
#include "MemoryRepositoryDiscussionThread.h"
#include "MemoryRepositoryDiscussionThreadMessage.h"
#include "MemoryRepositoryDiscussionTag.h"
#include "MemoryRepositoryDiscussionCategory.h"
#include "MemoryRepositoryAttachment.h"
#include "MemoryRepositoryAuthorization.h"
#include "MemoryRepositoryStatistics.h"
#include "MetricsRepository.h"
#include "ContextProviderMocks.h"
#include "CommandHandler.h"
#include "Configuration.h"
#include "StringHelpers.h"
#include "EventObserver.h"
#include "EventImporter.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

struct CleanupFixture
{
    ~CleanupFixture()
    {
        Forum::Helpers::cleanupStringHelpers();

        //clean up resources cached by ICU so that they don't show up as memory leaks
        u_cleanup();
    }
};

template<typename Duration = std::chrono::microseconds, typename Action>
auto countDuration(Action&& action)
{

    auto start = std::chrono::high_resolution_clock::now();
    action();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<Duration>(end - start).count();
}

using namespace Forum;
using namespace Forum::Commands;
using namespace Forum::Repository;
using namespace Forum::Authorization;
using namespace Forum::Persistence;

struct IdType
{
    IdType() = default;
    IdType(const IdType&) = default;
    IdType(IdType&&) = default;

    IdType& operator=(const IdType&) = default;
    IdType& operator=(IdType&&) = default;

    IdType(const Helpers::UuidString& uuid)
    {
        uuid.toStringCompact(data.data());
    }

    std::array<char, Helpers::UuidString::StringRepresentationSizeCompact> data;
    operator StringView() const
    {
        return StringView(data.data(), data.size());
    }
    operator Entities::IdType() const
    {
        return Entities::IdType(std::string_view(data.data(), data.size()));
    }
};

struct BenchmarkContext
{
    std::shared_ptr<Entities::EntityCollection> entityCollection;
    std::shared_ptr<CommandHandler> handler;
    std::vector<IdType> userIds;
    std::vector<IdType> threadIds;
    std::vector<IdType> threadMessageIds;
    std::vector<IdType> tagIds;
    std::vector<IdType> categoryIds;
    const int timestampIncrementMultiplier = 2;
    std::shared_ptr<EventObserver> persistenceObserver;
    ObservableRepositoryRef observableRepository;
    DirectWriteRepositoryCollection writeRepositories;
    std::string importFromFolder;
    std::string exportToFolder;
    std::string messagesFile;
    bool onlyPopulateData{ false };
    bool promptBeforeStart{ false };
    bool promptBeforeBenchmark{ false };
    bool abortOnExit{ false };
    int parseCommandLineResult{};

    auto incrementTimestamp(int value)
    {
        return _currentTimestamp += value * timestampIncrementMultiplier;
    }

    auto currentTimestamp() const
    {
        return _currentTimestamp;
    }

private:
    Entities::Timestamp _currentTimestamp = 946684800; //2000-01-01
};

int parseCommandLineArgs(BenchmarkContext& context, int argc, const char* argv[]);

BenchmarkContext createContext(int argc, const char* argv[])
{
    BenchmarkContext context;
    context.parseCommandLineResult = parseCommandLineArgs(context, argc, argv);
    if (context.parseCommandLineResult)
    {
        return context;
    }

    auto entityCollection = std::make_shared<Entities::EntityCollection>(context.messagesFile);
    auto store = std::make_shared<MemoryStore>(entityCollection);

    auto authorization = std::make_shared<DefaultAuthorization>(entityCollection->grantedPrivileges(),
                                                                *entityCollection, true);

    auto authorizationRepository = std::make_shared<MemoryRepositoryAuthorization>(
        store, authorization, authorization, authorization, authorization, authorization);

    auto userRepository = std::make_shared<MemoryRepositoryUser>(store, authorization, authorizationRepository);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store, authorization,
                                                                                         authorizationRepository);
    auto discussionThreadMessageRepository = std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store, 
                                                                                                       authorization,
                                                                                                       authorizationRepository);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store, authorization);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store, authorization);
    auto attachmentRepository = std::make_shared<MemoryRepositoryAttachment>(store, authorization);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store, authorization);
    auto metricsRepository = std::make_shared<MetricsRepository>(store, authorization);


    context.entityCollection = entityCollection;
    context.observableRepository = userRepository;

    context.handler = std::make_shared<CommandHandler>(context.observableRepository, userRepository,
        discussionThreadRepository, discussionThreadMessageRepository, discussionTagRepository,
        discussionCategoryRepository, attachmentRepository, authorizationRepository, statisticsRepository, 
        metricsRepository);

    context.writeRepositories.user = userRepository;
    context.writeRepositories.discussionThread = discussionThreadRepository;
    context.writeRepositories.discussionThreadMessage = discussionThreadMessageRepository;
    context.writeRepositories.discussionTag = discussionTagRepository;
    context.writeRepositories.discussionCategory = discussionCategoryRepository;
    context.writeRepositories.attachment = attachmentRepository;
    context.writeRepositories.authorization = authorizationRepository;

    if (context.exportToFolder.size() > 0)
    {
        context.persistenceObserver = std::make_shared<EventObserver>(context.observableRepository->readEvents(),
                                                                      context.observableRepository->writeEvents(),
                                                                      context.exportToFolder, 3600);
    }
    return context;
}

/**
 * Preserve the same vector of parameter so as to not reallocate memory for it each time
 */
static std::vector<StringView> parametersVector(10);

IdType executeAndGetId(CommandHandler& handler, Command command,
                       const std::initializer_list<StringView>& parameters = {})
{
    parametersVector.clear();
    parametersVector.insert(parametersVector.end(), parameters.begin(), parameters.end());

    auto output = handler.handle(command, parametersVector);
    auto idStart = output.output.begin() + output.output.find("\"id\":\"") + 6;

    IdType result;
    std::copy(idStart, idStart + result.data.size(), result.data.begin());

    return result;
}

bool executeAndGetOk(CommandHandler& handler, Command command, const std::initializer_list<StringView>& parameters = {})
{
    parametersVector.clear();
    parametersVector.insert(parametersVector.end(), parameters.begin(), parameters.end());

    auto output = handler.handle(command, parametersVector);
    return output.statusCode == StatusCode::OK;
}

template<typename CommandType>
void execute(CommandHandler& handler, CommandType command, const std::initializer_list<StringView>& parameters = {})
{
    parametersVector.clear();
    parametersVector.insert(parametersVector.end(), parameters.begin(), parameters.end());

    handler.handle(command, parametersVector);
}

const int nrOfUsers = 1000;
const int nrOfThreads = nrOfUsers * 1;
const int maxThreadPinDisplayOrder = 10;
const float threadPinProbability = 0.01f;
const int nrOfMessages = nrOfThreads * 50;
const int nrOfVotes = nrOfMessages;
const float upVoteProbability = 0.75;
const int nrOfTags = 100;
const int nrOfCategories = 100;
const int nrOfCategoryParentChildRelationships = 70;
const int nrOfTagsPerCategoryMin = 1;
const int nrOfTagsPerCategoryMax = 4;
const int nrOfTagsPerThreadMin = 1;
const int nrOfTagsPerThreadMax = 4;
const float messageContentLengthMean = 1000;
const float messageContentLengthStddev = 200;
const int retries = 1000;

static std::atomic_int currentAuthNumber{ 1 };

std::random_device device;
std::mt19937 randomGenerator(device());

void showEntitySizes();

void populateData(BenchmarkContext& context);
void generateRandomData(BenchmarkContext& context);
void importPersistedData(BenchmarkContext& context);
void doBenchmarks(BenchmarkContext& context);

std::string getNewAuth()
{
    return std::string("auth-") + std::to_string(currentAuthNumber++);
}

int parseCommandLineArgs(BenchmarkContext& context, int argc, const char* argv[])
{
    boost::program_options::options_description options("Available options");
    options.add_options()
        ("help,h", "Display available options")
        ("onlyPopulateData,o", "Only loads data from a file or by random generation")
        ("promptBeforeStart,s", "Prompt the user to continue before starting the data population")
        ("promptBeforeBenchmark,p", "Prompt the user to continue before starting the benchmark")
        ("abort,a", "Abort on exit to prevent calling destructors")
        ("import-folder,i", boost::program_options::value<std::string>(), "Import events from folder")
        ("export-folder,e", boost::program_options::value<std::string>(), "Export events to folder")
        ("messages-file,m", boost::program_options::value<std::string>(), "Map messages from file");

    boost::program_options::variables_map arguments;

    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), arguments);
        boost::program_options::notify(arguments);
    }
    catch (std::exception& ex)
    {
        std::cerr << "Invalid command line: " << ex.what() << '\n';
        return 1;
    }

    if (arguments.count("help"))
    {
        std::cout << options << '\n';
        return 1;
    }

    context.onlyPopulateData = arguments.count("onlyPopulateData") > 0;
    context.promptBeforeStart = arguments.count("promptBeforeStart") > 0;
    context.promptBeforeBenchmark = arguments.count("promptBeforeBenchmark") > 0;
    context.abortOnExit = arguments.count("abort") > 0;

    if (arguments.count("import-folder"))
    {
        context.importFromFolder = arguments["import-folder"].as<std::string>();
    }

    if (arguments.count("export-folder"))
    {
        context.exportToFolder = arguments["export-folder"].as<std::string>();
    }

    if (arguments.count("messages-file"))
    {
        context.messagesFile = arguments["messages-file"].as<std::string>();
    }

    return 0;
}

int main(int argc, const char* argv[])
{
    CleanupFixture _;

    auto context = createContext(argc, argv);
    if (context.parseCommandLineResult)
    {
        return context.parseCommandLineResult;
    }
    showEntitySizes();

    if (context.promptBeforeStart)
    {
        std::string line;
        std::cout << "\nPress [ENTER] to start the data population\n";
        std::getline(std::cin, line);
    }

    auto populationDuration = countDuration<std::chrono::milliseconds>([&]()
    {
        context.entityCollection->startBatchInsert();
        populateData(context);
        context.entityCollection->stopBatchInsert();
    });

    std::cout << "Populate duration: " << populationDuration << " ms\n";

    if (context.onlyPopulateData)
    {
        if (context.abortOnExit)
        {
            std::abort();
        }
        else
        {
            return 0;
        }
    }

    std::cout << "=====\n";
    std::cout << "Forum Memory Repository Benchmarks\n";
    std::cout << "=====\n\n";

    if (context.importFromFolder.size() < 1)
    {
        std::cout << "# of users: " << nrOfUsers << '\n';
        std::cout << "# of discussion threads: " << nrOfThreads << '\n';
        std::cout << "# of discussion messages: " << nrOfMessages << '\n';
        std::cout << "\tDiscussion message length: mean = " << messageContentLengthMean <<
                                              ", stddev = " << messageContentLengthStddev << "\n\n";
        std::cout << "# of discussion tags: " << nrOfTags << '\n';
        std::cout << "# of discussion categories: " << nrOfCategories
                  << " (" << nrOfCategoryParentChildRelationships << " parent-child)\n";
        std::cout << "# of discussion tags/category: " << nrOfTagsPerCategoryMin << "-" << nrOfTagsPerCategoryMax << '\n';
        std::cout << "# of discussion tags/thread: " << nrOfTagsPerThreadMin << "-" << nrOfTagsPerThreadMax << '\n';
    }

    if (context.promptBeforeBenchmark)
    {
        std::string line;
        std::cout << "\nPress [ENTER] to start the benchmark\n";
        std::getline(std::cin, line);
    }

    doBenchmarks(context);

    if (context.abortOnExit)
    {
        std::abort();
    }
}

void showEntitySizes()
{
    std::cout << std::setfill(' ');
    std::cout << "Forum Entity Sizes:                   [bytes]\n";
    std::cout << "----                                   -----\n";
    std::cout << "User                                   " << std::setw(5) << sizeof(Entities::User) << '\n';
    std::cout << "DiscussionThread                       " << std::setw(5) << sizeof(Entities::DiscussionThread) << '\n';
    std::cout << "DiscussionThreadMessage                " << std::setw(5) << sizeof(Entities::DiscussionThreadMessage) << '\n';
    std::cout << "DiscussionTag                          " << std::setw(5) << sizeof(Entities::DiscussionTag) << '\n';
    std::cout << "DiscussionCategory                     " << std::setw(5) << sizeof(Entities::DiscussionCategory) << '\n';
    std::cout << "MessageComment                         " << std::setw(5) << sizeof(Entities::MessageComment) << '\n';
    std::cout << "-\n";
    std::cout << "IdType                                 " << std::setw(5) << sizeof(Entities::IdType) << '\n';
    std::cout << "Timestamp                              " << std::setw(5) << sizeof(Entities::Timestamp) << '\n';
    std::cout << "VisitDetails                           " << std::setw(5) << sizeof(Entities::VisitDetails) << '\n';
    std::cout << "LastUpdatedInfo                        " << std::setw(5) << sizeof(Entities::LastUpdatedInfo) << '\n';
    std::cout << "WholeChangeableString                  " << std::setw(5) << sizeof(Helpers::WholeChangeableString) << '\n';
    std::cout << "Json::JsonReadyString<4>               " << std::setw(5) << sizeof(Json::JsonReadyString<4>) << '\n';
    std::cout << "bool                                   " << std::setw(5) << sizeof(bool) << '\n';
    std::cout << "std::string                            " << std::setw(5) << sizeof(std::string) << '\n';
    std::cout << "VoteCollection                         " << std::setw(5) << sizeof(Entities::DiscussionThreadMessage::VoteCollection) << '\n';
    std::cout << "AttachmentCollection                   " << std::setw(5) << sizeof(Entities::DiscussionThreadMessage::AttachmentCollection) << '\n';
    std::cout << "std::unique_ptr<VoteCollection>        " << std::setw(5) << sizeof(std::unique_ptr<Entities::DiscussionThreadMessage::VoteCollection>) << '\n';
    std::cout << "-\n";
    std::cout << "UserCollection                         " << std::setw(5) << sizeof(Entities::UserCollection) << '\n';
    std::cout << "DiscussionThreadCollectionHash         " << std::setw(5) << sizeof(Entities::DiscussionThreadCollectionWithHashedId) << '\n';
    std::cout << "DiscussionThreadCollectionLowMemory    " << std::setw(5) << sizeof(Entities::DiscussionThreadCollectionLowMemory) << '\n';
    std::cout << "DiscussionThreadMessageCollection      " << std::setw(5) << sizeof(Entities::DiscussionThreadMessageCollection) << '\n';
    std::cout << "DiscussionTagCollection                " << std::setw(5) << sizeof(Entities::DiscussionTagCollection) << '\n';
    std::cout << "DiscussionCategoryCollection           " << std::setw(5) << sizeof(Entities::DiscussionCategoryCollection) << '\n';
    std::cout << "MessageCommentCollection               " << std::setw(5) << sizeof(Entities::MessageCommentCollection) << '\n';
    std::cout << "-\n";
    std::cout << "PrivilegeValueType                     " << std::setw(5) << sizeof(Authorization::PrivilegeValueType) << '\n';
    std::cout << "DiscussionThreadMessagePrivilegeStore  " << std::setw(5) << sizeof(Authorization::DiscussionThreadMessagePrivilegeStore) << '\n';
    std::cout << "DiscussionThreadPrivilegeStore         " << std::setw(5) << sizeof(Authorization::DiscussionThreadPrivilegeStore) << '\n';
    std::cout << "DiscussionTagPrivilegeStore            " << std::setw(5) << sizeof(Authorization::DiscussionTagPrivilegeStore) << '\n';
    std::cout << "=====\n";
    std::cout << std::setw(0);
}

StringView getRandomText(char* buffer, size_t size)
{
    static std::uniform_int_distribution<> lowercaseAsciiDistribution('a', 'z');

    auto result = StringView(buffer, size);

    for (size_t i = 0; i < size; ++i)
    {
        *buffer++ = static_cast<char>(lowercaseAsciiDistribution(randomGenerator));
    }

    return result;
}

StringView getMessageText(char* buffer, size_t size)
{
    //randomness is not that important
    static const char characters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    constexpr size_t charactersCount = std::size(characters) - 1;

    static size_t startIndex = 0;
    ++startIndex;

    auto result = StringView(buffer, size);
    auto offset = startIndex;

    while (size > 0)
    {
        offset %= charactersCount;
        const auto toCopy = std::min(size, charactersCount - offset);

        memcpy(buffer, characters + offset, toCopy);
        buffer += toCopy;
        offset += toCopy;
        size -= toCopy;
    }

    return result;
}

size_t appendUInt(char* string, unsigned int value)
{
    char buffer[40];
    auto start = std::end(buffer) - 1;

    *start-- = '0' + (value % 10);

    while (value > 9)
    {
        value /= 10;
        *start-- = '0' + (value % 10);
    }

    start += 1;

    std::copy(start, std::end(buffer), string);

    return std::end(buffer) - start;
}

void populateData(BenchmarkContext& context)
{
    if (context.importFromFolder.empty())
    {
        generateRandomData(context);
    }
    else
    {
        importPersistedData(context);
    }
}

void generateRandomData(BenchmarkContext& context)
{
    auto& handler = *context.handler;
    auto& userIds = context.userIds;
    auto& threadIds = context.threadIds;
    auto& threadMessageIds = context.threadMessageIds;
    auto& tagIds = context.tagIds;
    auto& categoryIds = context.categoryIds;

    auto getCurrentTimestamp = [&context]() { return context.currentTimestamp(); };
    Context::setCurrentTimeMockForCurrentThread(getCurrentTimestamp);

    char buffer[8192];

    for (unsigned int i = 0; i < nrOfUsers; i++)
    {
        getRandomText(buffer, 5);
        const auto intSize = appendUInt(buffer + 5, i + 1);
        Context::setCurrentUserAuth(getNewAuth());
        userIds.emplace_back(executeAndGetId(handler, Command::ADD_USER, { StringView(buffer, 5 + intSize) }));
        Context::setCurrentUserAuth({});
        context.incrementTimestamp(100);
    }

    std::uniform_int_distribution<> userIdDistribution(0, static_cast<int>(userIds.size() - 1));
    std::normal_distribution<float> messageSizedistribution(messageContentLengthMean, messageContentLengthStddev);

    auto config = Configuration::getGlobalConfig();


    auto addMessage = [&](const IdType& threadId)
    {
        auto messageLength = static_cast<int_fast32_t>(messageSizedistribution(randomGenerator));
        messageLength = std::max(config->discussionThreadMessage.minContentLength, messageLength);
        messageLength = std::min(config->discussionThreadMessage.maxContentLength, messageLength);
        messageLength = std::min(static_cast<decltype(messageLength)>(4095), messageLength);

        threadMessageIds.emplace_back(executeAndGetId(handler, Command::ADD_DISCUSSION_THREAD_MESSAGE,
                                                      { threadId, getMessageText(buffer, messageLength) }));
    };

    for (size_t i = 0; i < nrOfTags; i++)
    {
        tagIds.emplace_back(executeAndGetId(handler, Command::ADD_DISCUSSION_TAG, { "Tag" + std::to_string(i + 1) }));
        context.incrementTimestamp(100);
    }

    std::uniform_int_distribution<> tagIdDistribution(0, static_cast<int>(tagIds.size() - 1));
    std::uniform_int_distribution<> nrOfTagsPerCategoryDistribution(nrOfTagsPerCategoryMin, nrOfTagsPerCategoryMax);
    std::uniform_int_distribution<> nrOfTagsPerThreadDistribution(nrOfTagsPerThreadMin, nrOfTagsPerThreadMax);

    std::vector<std::tuple<IdType, IdType>> threadTagsToAdd;

    int messagesProcessed = 0;
    int messagesProcessedPercent = -1;

    auto updateMessagesProcessedPercent = [&messagesProcessed, &messagesProcessedPercent]()
    {
        messagesProcessed += 1;
        int newPercent = messagesProcessed * 100 / nrOfMessages;
        if (newPercent > messagesProcessedPercent)
        {
            messagesProcessedPercent = newPercent;
            if (0 == newPercent)
            {
                std::cout << "Adding threads and messages... ";
            }
            std::cout << messagesProcessedPercent << "% ";
            std::cout.flush();
            if (100 == newPercent)
            {
                std::cout << std::endl;
            }
        }
    };

    std::uniform_int_distribution<> threadPinDisplayOrderDistribution(1, maxThreadPinDisplayOrder);
    std::uniform_real_distribution<> threadPinDistribution(0.0, 1.0);

    for (size_t i = 0; i < nrOfThreads; i++)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);
        auto id = executeAndGetId(handler, Command::ADD_DISCUSSION_THREAD, { getRandomText(buffer, 50) });

        for (int j = 0, n = nrOfTagsPerThreadDistribution(randomGenerator); j < n; ++j)
        {
            threadTagsToAdd.push_back(std::make_tuple(tagIds[tagIdDistribution(randomGenerator)], id));
        }

        threadIds.emplace_back(id);
        addMessage(id);

        if (threadPinDistribution(randomGenerator) < threadPinProbability)
        {
            execute(handler, Command::CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER,
                    { id, std::to_string(threadPinDisplayOrderDistribution(randomGenerator)) });
        }

        context.incrementTimestamp(100);
        updateMessagesProcessedPercent();
    }
    std::uniform_int_distribution<> threadIdDistribution(0, static_cast<int>(threadIds.size() - 1));

    for (size_t i = 0; i < (nrOfMessages - nrOfThreads); i++)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);
        addMessage(threadIds[threadIdDistribution(randomGenerator)]);

        context.incrementTimestamp(10);
        updateMessagesProcessedPercent();
    }

    std::uniform_int_distribution<> threadMessageIdDistribution(0, static_cast<int>(threadMessageIds.size() - 1));
    std::uniform_real_distribution<> upOrDownVoteDistribution(0.0, 1.0);

    for (size_t i = 0; i < nrOfVotes; i++)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);

        if (upOrDownVoteDistribution(randomGenerator) < upVoteProbability)
        {
            execute(handler, Command::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                    { threadMessageIds[threadMessageIdDistribution(randomGenerator)] });
        }
        else
        {
            execute(handler, Command::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                    { threadMessageIds[threadMessageIdDistribution(randomGenerator)] });
        }

        context.incrementTimestamp(1);
    }

    for (auto& tuple : threadTagsToAdd)
    {
        execute(handler, Command::ADD_DISCUSSION_TAG_TO_THREAD, { std::get<0>(tuple), std::get<1>(tuple) });
    }

    for (size_t i = 0; i < nrOfCategories; i++)
    {
        auto id = executeAndGetId(handler, Command::ADD_DISCUSSION_CATEGORY, { "Category" + std::to_string(i + 1) });
        categoryIds.emplace_back(id);
        execute(handler, Command::CHANGE_DISCUSSION_CATEGORY_DESCRIPTION, { id, "Description for Category" + std::to_string(i + 1) });
        for (int j = 0, n = nrOfTagsPerCategoryDistribution(randomGenerator); j < n; ++j)
        {
            execute(handler, Command::ADD_DISCUSSION_TAG_TO_CATEGORY, { tagIds[tagIdDistribution(randomGenerator)], id });
        }
        context.incrementTimestamp(100);
    }
    std::uniform_int_distribution<> categoryIdDistribution(0, static_cast<int>(categoryIds.size() - 1));

    int addedParentChildRelationships = 0;
    while (addedParentChildRelationships < nrOfCategoryParentChildRelationships)
    {
        auto& categoryParent = categoryIds[categoryIdDistribution(randomGenerator)];
        auto& categoryChild = categoryIds[categoryIdDistribution(randomGenerator)];

        if (executeAndGetOk(handler, Command::CHANGE_DISCUSSION_CATEGORY_PARENT, { categoryChild, categoryParent }))
        {
            addedParentChildRelationships += 1;
        }
    }
}

void importPersistedData(BenchmarkContext& context)
{
    EventImporter importer(false, *context.entityCollection, context.writeRepositories);
    if ( ! importer.import(context.importFromFolder).success)
    {
        std::abort();
    }

    //fill context ids as they are needed by doBenchmarks()
    for (auto& user : context.entityCollection->users().byId())
    {
        context.userIds.push_back(user->id());
    }

    context.entityCollection->threads().iterateThreads([&context](Entities::DiscussionThreadPtr threadPtr)
    {
        context.threadIds.push_back(threadPtr->id());        
    });
    for (auto& tag : context.entityCollection->tags().byId())
    {
        context.tagIds.push_back(tag->id());
    }
    for (auto& category : context.entityCollection->categories().byId())
    {
        context.categoryIds.push_back(category->id());
    }

    currentAuthNumber = static_cast<int>(context.userIds.size()) + 1000;

    std::cout << "---\n";
    std::cout << "Imported:\n";
    std::cout << "    Users: " << context.userIds.size() << "\n";
    std::cout << "    Discussion threads: " << context.threadIds.size() << "\n";
    std::cout << "    Discussion thread messages: " << context.entityCollection->threadMessages().count() << "\n";
    std::cout << "    Discussion tags: " << context.tagIds.size() << "\n";
    std::cout << "    Discussion categories: " << context.categoryIds.size() << "\n";
    std::cout << "---\n";
}

void doBenchmarks(BenchmarkContext& context)
{
    std::cout << "Results [microseconds]\n";
    std::cout << "-----\n\n";

    auto& handler = *context.handler;
    auto& userIds = context.userIds;
    auto& threadIds = context.threadIds;
    auto& tagIds = context.tagIds;
    auto& categoryIds = context.categoryIds;

    std::cout << "Adding a new user: ";

    for (int i = 0; i < retries; ++i)
    {
        std::cout << countDuration([&]()
        {
            Context::setCurrentUserAuth(getNewAuth());
            execute(handler, Command::ADD_USER, { "User" + std::to_string(i + 1) });
            Context::setCurrentUserAuth({});
        }) << " ";
        context.incrementTimestamp(100);
    }
    std::cout << '\n';

    std::uniform_int_distribution<> userIdDistribution(0, static_cast<int>(userIds.size() - 1));
    std::uniform_int_distribution<> threadIdDistribution(0, static_cast<int>(threadIds.size() - 1));
    std::uniform_int_distribution<> tagIdDistribution(0, static_cast<int>(tagIds.size() - 1));
    std::uniform_int_distribution<> categoryIdDistribution(0, static_cast<int>(categoryIds.size() - 1));

    char buffer[8192];

    std::cout << "Adding a new discussion thread: ";
    for (int i = 0; i < retries; ++i)
    {
        std::cout << countDuration([&]()
        {
            execute(handler, Command::ADD_DISCUSSION_THREAD, { getRandomText(buffer, 50) });
        }) << " ";
        context.incrementTimestamp(10);
    }
    std::cout << '\n';

    std::cout << "Adding a new message to an existing discussion thread: ";
    const std::string sampleMessage = "wmahcgobadxjrtbzoryzdskvxzidmjunsfjrajqljjtyhpgmhbtdrpqbkirlrowssftocsilbycloqxlhxpdlhnxnpxikkmbswckpoxijljjdqmfmdorehztywtcsvbcasnpksnwbmjztxoqxogfjmxwuymkhxzzjqtytmtqxdizxtjqqscczyhssnnucttrjdxzibrgihojzwcgsuwxboumqzqmlsjxxnclqpmsjkqsqvhgyzhpoyhtotilggkxyojwbefizlexbgtswxwjqjohlaeexzxcwtpikfluvqhxqsqlnamaytnmxtazzbvmdykeyvsihcpngnmnwchmpfzrwsjngtmykcyzazsbpmaymejmxjrjpcltdixesatxpstjffjwtsysswnyrzycamsimtzfqkickbohwgpsyvpbvuytoxrcicfzpiiaygoansusdymdelglbclljnpzhqzfsklepvdhtejdptwwpyxwibgjgvcylcdtzcoqzaouqgnobhmywvcskqcpmaquqzirymnfxvmmxyvvohzchiotnztbfocqsueriwedyyqwlimbqjcxvbxlfdorqoriehywuprfnubxdskvprfkpvgxyaqfnuuqpghpdypiuqmcmtslinlbobbqumrcbyoczdsajfhcsidgwsrfqmzasefyomizcuuqttioxxintwzrysjqqkpkyrawtxjvyaapmghpykwbnepfsozmngkwapmwqhketucpgxkfpmorssyjftqsytqchnnedgbgasqylszuqmeezsihxdqtqxgqndflxwetbkwwgontycfizbgyzefzqwcffqewaxdronkeitbwuujxkvvpdqrjyujbznpvtkibzpumyhtpfkxnabpookgqpkgrkjuznklokqwngtqumdmzttixjncjjqemsdhenlfmdqfpbbrvgzrhnqdzgaygbfwukljhwwvoddltjriuztdsolssyyosymqooeucdqqjbjgqzqdcbfataqjggjmjaroaaanjqdeesnfnjxagylhswcufxinzwvrxrpqhtbkzosukhfvvtfusklappmtkvvsrfohvdylvhggbsuempkyruiwhtzqelvwmnmdtbdtaqqgxrqyyivdrjjdxztpxgkseohgbjdqdtcpndm";
    for (int i = 0; i < retries; ++i)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);

        auto& threadId = threadIds[threadIdDistribution(randomGenerator)];

        std::cout << countDuration([&]()
        {
            execute(handler, Command::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, sampleMessage });
        }) << " ";
        context.incrementTimestamp(10);
    }
    std::cout << "\n\n";

    std::cout << "Get first page of users by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_USERS_BY_NAME);
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get fourth page of users by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 3;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_USERS_BY_NAME);
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get first page of users by last seen: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_USERS_BY_LAST_SEEN);
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get fourth page of users by last seen: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 3;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_USERS_BY_LAST_SEEN);
        }) << " ";
    }
    std::cout << '\n';


    std::cout << "Get first page of discussion threads by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_BY_NAME);
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get fourth page of discussion threads by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 3;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_BY_NAME);
        }) << " ";
    }
    std::cout << '\n';


    std::cout << "Get first page of discussion threads by message count descending: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Descending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get fourth page of discussion threads by message count descending: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 3;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Descending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
        }) << " ";
    }
    std::cout << '\n';


    std::cout << "Get first page of discussion threads of user by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                    { userIds[userIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get first page of discussion thread messages: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREAD_BY_ID,
                    { threadIds[threadIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';


    std::cout << "Get first page of discussion threads with tag by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
                    { tagIds[tagIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get first page of discussion threads of category by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                    { categoryIds[categoryIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get second page of discussion threads of category by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 1;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                    { categoryIds[categoryIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get first page of discussion thread messages of user by created: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, View::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,
                    { userIds[userIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Merge all tags: ";
    for (size_t i = 1; i < tagIds.size(); ++i)
    {
        std::cout << countDuration([&]()
        {
            execute(handler, Command::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG, { tagIds[i], tagIds[0] });
        }) << " " << std::flush;
    }
    std::cout << '\n';
}
