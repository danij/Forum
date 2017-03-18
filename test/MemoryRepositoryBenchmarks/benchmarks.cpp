#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>

#include <unicode/uclean.h>

#include "MemoryRepositoryCommon.h"
#include "MemoryRepositoryUser.h"
#include "MemoryRepositoryDiscussionThread.h"
#include "MemoryRepositoryDiscussionThreadMessage.h"
#include "MemoryRepositoryDiscussionTag.h"
#include "MemoryRepositoryDiscussionCategory.h"
#include "MemoryRepositoryStatistics.h"
#include "MetricsRepository.h"
#include "ContextProviderMocks.h"
#include "CommandHandler.h"
#include "Configuration.h"
#include "StringHelpers.h"

#include <boost/property_tree/json_parser.hpp>

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

struct IdType
{
    std::array<char, 36> data;
    operator StringView() const
    {
        return StringView(data.data(), data.size());
    }
    operator Entities::IdType() const
    {
        return Entities::IdType(boost::string_view(data.data(), data.size()));
    }
};

struct BenchmarkContext
{
    std::shared_ptr<CommandHandler> handler;
    std::vector<IdType> userIds;
    std::vector<IdType> threadIds;
    std::vector<IdType> tagIds;
    std::vector<IdType> categoryIds;
    Entities::Timestamp currentTimestamp = 1000;
};

auto createCommandHandler()
{
    auto store = std::make_shared<MemoryStore>(std::make_shared<Entities::EntityCollection>());
    auto userRepository = std::make_shared<MemoryRepositoryUser>(store);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store);
    auto discussionThreadMessageRepository = std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store);
    auto metricsRepository = std::make_shared<MetricsRepository>();

    ObservableRepositoryRef observableRepository = userRepository;

    BenchmarkContext context;
    context.handler = std::make_shared<CommandHandler>(observableRepository, userRepository, discussionThreadRepository,
        discussionThreadMessageRepository, discussionTagRepository, discussionCategoryRepository,
        statisticsRepository, metricsRepository);

    return context;
}

IdType executeAndGetId(CommandHandler& handler, Command command, const std::vector<StringView>& parameters = {})
{
    auto output = handler.handle(command, parameters);
    auto idStart = output.output.begin() + output.output.find("\"id\":\"") + 6;

    IdType result;
    std::copy(idStart, idStart + result.data.size(), result.data.begin());

    return result;
}

bool executeAndGetOk(CommandHandler& handler, Command command, const std::vector<StringView>& parameters = {})
{
    auto output = handler.handle(command, parameters);
    return output.statusCode == StatusCode::OK;
}

inline void execute(CommandHandler& handler, Command command, const std::vector<StringView>& parameters = {})
{
    handler.handle(command, parameters);
}

const int nrOfUsers = 10000;
const int nrOfThreads = nrOfUsers * 1;
const int nrOfMessages = nrOfThreads * 50;
const int nrOfTags = 100;
const int nrOfCategories = 100;
const int nrOfCategoryParentChildRelationships = 20;
const int nrOfTagsPerCategoryMin = 1;
const int nrOfTagsPerCategoryMax = 4;
const int nrOfTagsPerThreadMin = 1;
const int nrOfTagsPerThreadMax = 4;
const float messageContentLengthMean = 1000;
const float messageContentLengthStddev = 200;
const int retries = 1000;

std::random_device device;
std::mt19937 randomGenerator(device());

void showEntitySizes();

void populateData(BenchmarkContext& context);
void doBenchmarks(BenchmarkContext& context);

int main()
{
    CleanupFixture _;

    showEntitySizes();

    auto context = createCommandHandler();

    Context::mutableSkipValidations() = true;
    Context::mutableSkipObservers() = true;
    std::cout << "Populate duration: " << countDuration<std::chrono::milliseconds>([&]() { populateData(context); }) << " ms\n";
    Context::mutableSkipValidations() = false;
    Context::mutableSkipObservers() = false;

    std::cout << "=====\n";
    std::cout << "Forum Memory Repository Benchmarks\n";
    std::cout << "=====\n\n";

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

    //std::string line;
    //std::cout << "\nPress [ENTER] to start the benchmark\n";
    //std::getline(std::cin, line);

    doBenchmarks(context);
}

void showEntitySizes()
{
    std::cout << std::setfill(' ');
    std::cout << "Forum Entity Sizes:                   [bytes]\n";
    std::cout << "----                                   -----\n";
    std::cout << "Identifiable                           " << std::setw(5) << sizeof(Entities::Identifiable) << '\n';
    std::cout << "CreatedMixin                           " << std::setw(5) << sizeof(Entities::CreatedMixin) << '\n';
    std::cout << "LastUpdatedMixin<User>                 " << std::setw(5) << sizeof(Entities::LastUpdatedMixin<Entities::User>) << '\n';
    std::cout << "User                                   " << std::setw(5) << sizeof(Entities::User) << '\n';
    std::cout << "UserCollectionBase                     " << std::setw(5) << sizeof(Entities::UserCollectionBase<Entities::OrderedIndexForId>) << '\n';
    std::cout << "DiscussionThread                       " << std::setw(5) << sizeof(Entities::DiscussionThread) << '\n';
    std::cout << "DiscussionThreadCollectionBase         " << std::setw(5) << sizeof(Entities::DiscussionThreadCollectionBase<Entities::OrderedIndexForId>) << '\n';
    std::cout << "DiscussionThreadMessage                " << std::setw(5) << sizeof(Entities::DiscussionThreadMessage) << '\n';
    std::cout << "DiscussionThreadMessageCollectionBase  " << std::setw(5) << sizeof(Entities::DiscussionThreadMessageCollectionBase<Entities::OrderedIndexForId>) << '\n';
    std::cout << "MessageComment                         " << std::setw(5) << sizeof(Entities::MessageComment) << '\n';
    std::cout << "MessageCommentCollectionBase           " << std::setw(5) << sizeof(Entities::MessageCommentCollectionBase<Entities::OrderedIndexForId>) << '\n';
    std::cout << "DiscussionTag                          " << std::setw(5) << sizeof(Entities::DiscussionTag) << '\n';
    std::cout << "DiscussionTagCollectionBase            " << std::setw(5) << sizeof(Entities::DiscussionTagCollectionBase<Entities::OrderedIndexForId>) << '\n';
    std::cout << "DiscussionCategory                     " << std::setw(5) << sizeof(Entities::DiscussionCategory) << '\n';
    std::cout << "DiscussionCategoryCollectionBase       " << std::setw(5) << sizeof(Entities::DiscussionCategoryCollectionBase<Entities::OrderedIndexForId>) << '\n';
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
    constexpr size_t charactersCount = std::extent<decltype(characters)>::value - 1;

    static int startIndex = 0;
    ++startIndex;

    auto result = StringView(buffer, size);
    auto offset = startIndex;

    while (size > 0)
    {
        offset %= charactersCount;
        auto toCopy = std::min(size, charactersCount - offset);

        std::copy(characters + offset, characters + offset + toCopy, buffer);
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
    auto& handler = *context.handler;
    auto& userIds = context.userIds;
    auto& threadIds = context.threadIds;
    auto& tagIds = context.tagIds;
    auto& categoryIds = context.categoryIds;

    auto getCurrentTimestamp = [&context]() { return context.currentTimestamp; };
    Context::setCurrentTimeMockForCurrentThread(getCurrentTimestamp);

    char buffer[8192];

    for (unsigned int i = 0; i < nrOfUsers; i++)
    {
        getRandomText(buffer, 5);
        auto intSize = appendUInt(buffer + 5, i + 1);
        userIds.emplace_back(executeAndGetId(handler, Command::ADD_USER, { StringView(buffer, 5 + intSize) }));
        context.currentTimestamp += 100;
    }

    std::uniform_int_distribution<> userIdDistribution(0, userIds.size() - 1);
    std::normal_distribution<float> messageSizedistribution(messageContentLengthMean, messageContentLengthStddev);
    
    auto config = Configuration::getGlobalConfig();


    auto addMessage = [&](const IdType& threadId)
    {
        auto messageLength = static_cast<int_fast32_t>(messageSizedistribution(randomGenerator));
        messageLength = std::max(config->discussionThreadMessage.minContentLength, messageLength);
        messageLength = std::min(config->discussionThreadMessage.maxContentLength, messageLength);
        messageLength = std::min(static_cast<decltype(messageLength)>(4095), messageLength);

        execute(handler, Command::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, getMessageText(buffer, messageLength) });
    };

    for (size_t i = 0; i < nrOfTags; i++)
    {
        tagIds.emplace_back(executeAndGetId(handler, Command::ADD_DISCUSSION_TAG, { "Tag" + std::to_string(i + 1) }));
        context.currentTimestamp += 100;
    }

    std::uniform_int_distribution<> tagIdDistribution(0, tagIds.size() - 1);
    std::uniform_int_distribution<> nrOfTagsPerCategoryDistribution(nrOfTagsPerCategoryMin, nrOfTagsPerCategoryMax);
    std::uniform_int_distribution<> nrOfTagsPerThreadDistribution(nrOfTagsPerThreadMin, nrOfTagsPerThreadMax);

    std::vector<std::tuple<IdType, IdType>> threadTagsToAdd;

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

        context.currentTimestamp += 10;
    }
    std::uniform_int_distribution<> threadIdDistribution(0, threadIds.size() - 1);

    for (size_t i = 0; i < (nrOfMessages - nrOfThreads); i++)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);
        addMessage(threadIds[threadIdDistribution(randomGenerator)]);

        context.currentTimestamp += 1;
    }

    for (auto& tuple : threadTagsToAdd)
    {
        execute(handler, Command::ADD_DISCUSSION_TAG_TO_THREAD, { std::get<0>(tuple), std::get<1>(tuple) });
    }

    for (size_t i = 0; i < nrOfCategories; i++)
    {
        auto id = executeAndGetId(handler, Command::ADD_DISCUSSION_CATEGORY, { "Category" + std::to_string(i + 1) });
        categoryIds.emplace_back(id);
        for (int j = 0, n = nrOfTagsPerCategoryDistribution(randomGenerator); j < n; ++j)
        {
            execute(handler, Command::ADD_DISCUSSION_TAG_TO_CATEGORY, { tagIds[tagIdDistribution(randomGenerator)], id });
        }
        context.currentTimestamp += 100;
    }
    std::uniform_int_distribution<> categoryIdDistribution(0, categoryIds.size() - 1);

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
            execute(handler, Command::ADD_USER, { "User" + std::to_string(i + 1) });
        }) << " ";
        context.currentTimestamp += 100;
    }
    std::cout << '\n';

    std::uniform_int_distribution<> userIdDistribution(0, userIds.size() - 1);
    std::uniform_int_distribution<> threadIdDistribution(0, threadIds.size() - 1);
    std::uniform_int_distribution<> tagIdDistribution(0, tagIds.size() - 1);
    std::uniform_int_distribution<> categoryIdDistribution(0, categoryIds.size() - 1);

    char buffer[8192];

    std::cout << "Adding a new discussion thread: ";
    for (int i = 0; i < retries; ++i)
    {
        std::cout << countDuration([&]()
        {
            execute(handler, Command::ADD_DISCUSSION_THREAD, { getRandomText(buffer, 50) });
        }) << " ";
        context.currentTimestamp += 10;
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
        context.currentTimestamp += 10;
    }
    std::cout << "\n\n";

    std::cout << "Get first page of users by name: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, Command::GET_USERS_BY_NAME);
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
            execute(handler, Command::GET_USERS_BY_NAME);
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
            execute(handler, Command::GET_USERS_BY_LAST_SEEN);
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
            execute(handler, Command::GET_USERS_BY_LAST_SEEN);
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
            execute(handler, Command::GET_DISCUSSION_THREADS_BY_NAME);
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
            execute(handler, Command::GET_DISCUSSION_THREADS_BY_NAME);
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
            execute(handler, Command::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
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
            execute(handler, Command::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT);
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
            execute(handler, Command::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                    { userIds[userIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';

    std::cout << "Get first page of message of discussion threads: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::getMutableDisplayContext().pageNumber = 0;
        Context::getMutableDisplayContext().sortOrder = Context::SortOrder::Ascending;

        std::cout << countDuration([&]()
        {
            execute(handler, Command::GET_DISCUSSION_THREAD_BY_ID, 
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
            execute(handler, Command::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
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
            execute(handler, Command::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                    { categoryIds[categoryIdDistribution(randomGenerator)] });
        }) << " ";
    }
    std::cout << '\n';
}