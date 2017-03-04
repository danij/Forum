#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>

#include <unicode/uclean.h>

#include "MemoryRepository.h"
#include "MetricsRepository.h"
#include "CommandHandler.h"
#include "Configuration.h"
#include "StringHelpers.h"

#include <boost/property_tree/json_parser.hpp>
#include "ContextProviderMocks.h"

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
auto countDuration(Action action)
{

    auto start = std::chrono::high_resolution_clock::now();
    action();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<Duration>(end - start).count();
}

using namespace Forum;
using namespace Forum::Commands;
using namespace Forum::Repository;

struct BenchmarkContext
{
    std::shared_ptr<CommandHandler> handler;
    std::vector<std::string> userIds;
    std::vector<std::string> threadIds;
    std::vector<std::string> tagIds;
    std::vector<std::string> categoryIds;
    Entities::Timestamp currentTimestamp = 1000;
};

auto createCommandHandler()
{
    auto memoryRepository = std::make_shared<MemoryRepository>();
    auto metricsRepository = std::make_shared<MetricsRepository>();
    BenchmarkContext context;
    context.handler = std::make_shared<CommandHandler>(memoryRepository, memoryRepository, metricsRepository);
    return context;
}

std::string executeAndGetId(CommandHandler& handler, Command command, std::vector<std::string> parameters = {})
{
    std::stringstream stream;
    handler.handle(command, parameters, stream);
    boost::property_tree::ptree result;
    boost::property_tree::read_json(stream, result);
    return result.get<std::string>("id");
}

bool executeAndGetOk(CommandHandler& handler, Command command, std::vector<std::string> parameters = {})
{
    std::stringstream stream;
    handler.handle(command, parameters, stream);
    boost::property_tree::ptree result;
    boost::property_tree::read_json(stream, result);
    return result.get<uint32_t>("status") == StatusCode::OK;
}

inline void execute(CommandHandler& handler, Command command, std::vector<std::string> parameters = {})
{
    std::stringstream stream;
    handler.handle(command, parameters, stream);
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

void populateData(BenchmarkContext& context);
void doBenchmarks(BenchmarkContext& context);

int main()
{
    CleanupFixture _;

    auto context = createCommandHandler();

    std::cout << "Populate duration: " << countDuration<std::chrono::milliseconds>([&]() { populateData(context); }) << " ms\n";

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

char* getRandomText(size_t size)
{
    static std::uniform_int_distribution<> lowercaseAsciiDistribution('a', 'z');
    static char name[8192];

    size = std::min(size, sizeof(name) / sizeof(name[0]) - 1);
    for (size_t i = 0; i < size; ++i)
    {
        name[i] = static_cast<char>(lowercaseAsciiDistribution(randomGenerator));
    }
    name[size] = 0;

    return name;
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

    for (size_t i = 0; i < nrOfUsers; i++)
    {
        userIds.emplace_back(executeAndGetId(handler, Command::ADD_USER, { getRandomText(5) + std::to_string(i + 1) }));
        context.currentTimestamp += 100;
    }

    for (size_t i = 0; i < nrOfTags; i++)
    {
        tagIds.emplace_back(executeAndGetId(handler, Command::ADD_DISCUSSION_TAG, { "Tag" + std::to_string(i + 1) }));
        context.currentTimestamp += 100;
    }
    std::uniform_int_distribution<> tagIdDistribution(0, tagIds.size() - 1);
    std::uniform_int_distribution<> nrOfTagsPerCategoryDistribution(nrOfTagsPerCategoryMin, nrOfTagsPerCategoryMax);
    std::uniform_int_distribution<> nrOfTagsPerThreadDistribution(nrOfTagsPerThreadMin, nrOfTagsPerThreadMax);
    
    for (size_t i = 0; i < nrOfCategories; i++)
    {
        auto id = executeAndGetId(handler, Command::ADD_DISCUSSION_CATEGORY, { "Category" + std::to_string(i + 1) });
        categoryIds.emplace_back(id);
        for (int j = 0; j < nrOfTagsPerCategoryDistribution(randomGenerator); ++j)
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

    std::uniform_int_distribution<> userIdDistribution(0, userIds.size() - 1);
    std::normal_distribution<float> messageSizedistribution(messageContentLengthMean, messageContentLengthStddev);
    
    auto config = Configuration::getGlobalConfig();

    auto addMessage = [&](const std::string& threadId)
    {
        auto messageLength = static_cast<int_fast32_t>(messageSizedistribution(randomGenerator));
        messageLength = std::max(config->discussionThreadMessage.minContentLength, messageLength);
        messageLength = std::min(config->discussionThreadMessage.maxContentLength, messageLength);
        messageLength = std::min(static_cast<decltype(messageLength)>(4095), messageLength);

        execute(handler, Command::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, getRandomText(messageLength) });
    };

    for (size_t i = 0; i < nrOfThreads; i++)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);
        auto id = executeAndGetId(handler, Command::ADD_DISCUSSION_THREAD, { getRandomText(50) });

        for (int j = 0; j < nrOfTagsPerThreadDistribution(randomGenerator); ++j)
        {
            execute(handler, Command::ADD_DISCUSSION_TAG_TO_THREAD, { tagIds[tagIdDistribution(randomGenerator)], id });
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

    std::cout << "Adding a new discussion thread: ";
    for (int i = 0; i < retries; ++i)
    {
        std::cout << countDuration([&]()
        {
            execute(handler, Command::ADD_DISCUSSION_THREAD, { getRandomText(50) });
        }) << " ";
        context.currentTimestamp += 10;
    }
    std::cout << '\n';

    std::cout << "Adding a new message to an existing discussion thread: ";
    for (int i = 0; i < retries; ++i)
    {
        Context::setCurrentUserId(userIds[userIdDistribution(randomGenerator)]);

        const char* sampleMessage = "wmahcgobadxjrtbzoryzdskvxzidmjunsfjrajqljjtyhpgmhbtdrpqbkirlrowssftocsilbycloqxlhxpdlhnxnpxikkmbswckpoxijljjdqmfmdorehztywtcsvbcasnpksnwbmjztxoqxogfjmxwuymkhxzzjqtytmtqxdizxtjqqscczyhssnnucttrjdxzibrgihojzwcgsuwxboumqzqmlsjxxnclqpmsjkqsqvhgyzhpoyhtotilggkxyojwbefizlexbgtswxwjqjohlaeexzxcwtpikfluvqhxqsqlnamaytnmxtazzbvmdykeyvsihcpngnmnwchmpfzrwsjngtmykcyzazsbpmaymejmxjrjpcltdixesatxpstjffjwtsysswnyrzycamsimtzfqkickbohwgpsyvpbvuytoxrcicfzpiiaygoansusdymdelglbclljnpzhqzfsklepvdhtejdptwwpyxwibgjgvcylcdtzcoqzaouqgnobhmywvcskqcpmaquqzirymnfxvmmxyvvohzchiotnztbfocqsueriwedyyqwlimbqjcxvbxlfdorqoriehywuprfnubxdskvprfkpvgxyaqfnuuqpghpdypiuqmcmtslinlbobbqumrcbyoczdsajfhcsidgwsrfqmzasefyomizcuuqttioxxintwzrysjqqkpkyrawtxjvyaapmghpykwbnepfsozmngkwapmwqhketucpgxkfpmorssyjftqsytqchnnedgbgasqylszuqmeezsihxdqtqxgqndflxwetbkwwgontycfizbgyzefzqwcffqewaxdronkeitbwuujxkvvpdqrjyujbznpvtkibzpumyhtpfkxnabpookgqpkgrkjuznklokqwngtqumdmzttixjncjjqemsdhenlfmdqfpbbrvgzrhnqdzgaygbfwukljhwwvoddltjriuztdsolssyyosymqooeucdqqjbjgqzqdcbfataqjggjmjaroaaanjqdeesnfnjxagylhswcufxinzwvrxrpqhtbkzosukhfvvtfusklappmtkvvsrfohvdylvhggbsuempkyruiwhtzqelvwmnmdtbdtaqqgxrqyyivdrjjdxztpxgkseohgbjdqdtcpndm";
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
            execute(handler, Command::GET_DISCUSSION_THREAD_BY_ID, { threadIds[threadIdDistribution(randomGenerator)] });
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