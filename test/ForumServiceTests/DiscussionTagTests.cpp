#include "CommandsCommon.h"
#include "TestHelpers.h"

#include <vector>

using namespace Forum::Configuration;
using namespace Forum::Context;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

/**
* Stores only the information that is sent out about a discussion tag 
*/
struct SerializedDiscussionTag
{
    std::string id;
    std::string name;
    int64_t threadCount = 0;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        threadCount = tree.get<int64_t>("threadCount");
    }
};

static auto deserializeTag(const boost::property_tree::ptree& tree)
{
    SerializedDiscussionTag result;
    result.populate(tree);
    return result;
}

static auto deserializeTags(const boost::property_tree::ptree& collection)
{
    std::vector<SerializedDiscussionTag> result;
    for (auto& tree : collection)
    {
        result.push_back(deserializeTag(tree.second));
    }
    return result;
}

/**
* Stores only the information that is sent out about a user referenced in a discussion thread or message
*/
struct SerializedUserReferencedInDiscussionThreadOrMessage
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastSeen = 0;
    int threadCount = 0;
    int messageCount = 0;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastSeen = tree.get<Timestamp>("lastSeen");
        threadCount = tree.get<int>("threadCount");
        messageCount = tree.get<int>("messageCount");
    }
};

struct SerializedLatestDiscussionThreadMessage
{
    Timestamp created = 0;
    SerializedUserReferencedInDiscussionThreadOrMessage createdBy;

    void populate(const boost::property_tree::ptree& tree)
    {
        created = tree.get<Timestamp>("created");

        createdBy.populate(tree.get_child("createdBy"));
    }
};

struct SerializedDiscussionThread
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;
    SerializedUserReferencedInDiscussionThreadOrMessage createdBy;
    int64_t visited = 0;
    int64_t messageCount = 0;
    SerializedLatestDiscussionThreadMessage latestMessage;
    std::vector<SerializedDiscussionTag> tags;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastUpdated = tree.get<Timestamp>("lastUpdated");
        visited = tree.get<int64_t>("visited");
        messageCount = tree.get<int64_t>("messageCount");

        createdBy.populate(tree.get_child("createdBy"));
        for (auto& pair : tree)
        {
            if (pair.first == "latestMessage")
            {
                latestMessage.populate(pair.second);
            }
            else if (pair.first == "tags")
            {
                tags = deserializeTags(pair.second);
            }
        }
    }
};

static auto deserializeThread(const boost::property_tree::ptree& tree)
{
    SerializedDiscussionThread result;
    result.populate(tree);
    return result;
}

static auto deserializeThreads(const boost::property_tree::ptree& collection)
{
    std::vector<SerializedDiscussionThread> result;
    for (auto& tree : collection)
    {
        result.push_back(deserializeThread(tree.second));
    }
    return result;
}


BOOST_AUTO_TEST_CASE( No_discussion_tags_are_present_before_one_is_created )
{
    auto handler = createCommandHandler();
    auto threads = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));

    BOOST_REQUIRE_EQUAL(0, threads.size());

    threads = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT).get_child("tags"));

    BOOST_REQUIRE_EQUAL(0, threads.size());
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_returns_the_id_and_name )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { "Foo" });

    assertStatusCodeEqual(StatusCode::OK, returnObject);

    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_no_parameters_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG);
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_empty_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_only_whitespace_in_the_name_fails)
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_leading_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_trailing_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionTag.minNameLength - 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_a_too_long_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionTag.maxNameLength + 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_a_name_that_contains_invalid_characters_fails_with_appropriate_message )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_TAG, { "\xFF\xFF" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_multiple_discussion_tags_with_the_same_name_case_insensitive_fails )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG, { "Foo" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG, { "föo" }));
}

BOOST_AUTO_TEST_CASE( Renaming_a_discussion_tag_succeeds_only_if_creation_criteria_are_met )
{
    auto handler = createCommandHandler();
    auto tagId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG, { "Foo" }).get<std::string>("id");

    auto config = getGlobalConfig();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, "" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, " \t\r\n" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, " Foo" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, "Foo\t" }));
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, 
                                       { tagId, std::string(config->discussionTag.minNameLength - 1, 'a') }));
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, 
                                       { tagId, std::string(config->discussionTag.maxNameLength + 1, 'a') }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, "\xFF\xFF" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, "föo" }));
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_with_an_invalid_id_returns_invalid_parameters )
{
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(createCommandHandler(), Forum::Commands::DELETE_DISCUSSION_TAG, { "bogus id" }));
}

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_discussion_tag_returns_not_found )
{
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(createCommandHandler(), 
                                                              Forum::Commands::DELETE_DISCUSSION_TAG, 
                                                              { sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Deleted_discussion_tags_can_no_longer_be_retrieved )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };
    std::vector<std::string> ids;

    for (auto& name : names)
    {
        auto result = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG, { name });
        assertStatusCodeEqual(StatusCode::OK, result);
        ids.push_back(result.get<std::string>("id"));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionTags"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_TAG, { ids[0] }));
    
    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionTags"));

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));

    BOOST_REQUIRE_EQUAL(names.size() - 1, tags.size());
    BOOST_REQUIRE_EQUAL("Def", tags[0].name);
    BOOST_REQUIRE_EQUAL("Ghi", tags[1].name);
}

static Forum::Commands::Command GetDiscussionThreadWithTagCommands[] =
{
    Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
    Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED,
    Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED,
    Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT,
};

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_an_invalid_tag_fails )
{
    auto handler = createCommandHandler();
    for (auto command : GetDiscussionThreadWithTagCommands)
    {
        assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, command, { "bogus id" }));
    }
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_an_unknown_tag_returns_not_found )
{
    auto handler = createCommandHandler();
    for (auto command : GetDiscussionThreadWithTagCommands)
    {
        assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, command, { sampleValidIdString }));
    }
}

BOOST_AUTO_TEST_CASE( Discussion_threads_have_no_tags_attached_by_default )
{
    auto handler = createCommandHandler();
    auto tagId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG, { "Foo" }).get<std::string>("id");

    for (auto command : GetDiscussionThreadWithTagCommands)
    {
        auto threadsObj = handlerToObj(handler, command, { tagId });
        assertStatusCodeEqual(StatusCode::OK, threadsObj);

        auto threads = deserializeThreads(threadsObj.get_child("threads"));
        BOOST_REQUIRE_EQUAL(0, threads.size());
    }
}

BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_attached_to_threads_even_if_they_are_already_attached )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    for (size_t i = 0; i < 2; i++)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                           { tagId, threadId }));
    }

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME, 
                                                   { tagId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1, threads.size());
    BOOST_REQUIRE_EQUAL(threadId, threads[0].id);
    BOOST_REQUIRE_EQUAL("Thread", threads[0].name);
    BOOST_REQUIRE_EQUAL(1, threads[0].tags.size());
    BOOST_REQUIRE_EQUAL(tagId, threads[0].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag", threads[0].tags[0].name);
}

BOOST_AUTO_TEST_CASE( Attaching_discussion_tags_require_a_valid_discussion_tag_and_a_valid_thread )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                                       { "bogus tag id", "bogus thread id" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                                       { tagId, "bogus thread id" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                                       { "bogus tag id", threadId }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, 
                                                              Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                              { sampleValidIdString, sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, 
                                                              Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                              { tagId, sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, 
                                                              Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                              { sampleValidIdString, threadId }));
}

BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_detached_from_threads )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tagId, threadId }));

    auto threads = deserializeThreads(handlerToObj(handler,
                                                   Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
                                                   { tagId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1, threads.size());

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_THREAD,
                                                       { tagId, threadId }));

    threads = deserializeThreads(handlerToObj(handler,
                                              Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
                                              { tagId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(0, threads.size());
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_detaches_it_from_threads )
{
    auto handler = createCommandHandler();
    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    for (auto& threadId : { thread1Id, thread2Id })
        for (auto& tagId : { tag1Id, tag2Id })
        {
            assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                               { tagId, threadId }));
        }
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_TAG, { tag1Id }));

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));
    BOOST_REQUIRE_EQUAL(1, tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", tags[0].name);
    BOOST_REQUIRE_EQUAL(2, tags[0].threadCount);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                             .get_child("threads"));
    BOOST_REQUIRE_EQUAL(2, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL("Thread1", threads[0].name);
    BOOST_REQUIRE_EQUAL(1, threads[0].tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, threads[0].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", threads[0].tags[0].name);

    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
    BOOST_REQUIRE_EQUAL("Thread2", threads[1].name);
    BOOST_REQUIRE_EQUAL(1, threads[1].tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, threads[1].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", threads[1].tags[0].name);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_detaches_it_from_tags )
{
    auto handler = createCommandHandler();
    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    for (auto& threadId : { thread1Id, thread2Id })
        for (auto& tagId : { tag1Id, tag2Id })
        {
            assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                               { tagId, threadId }));
        }
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { thread1Id }));

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));
    BOOST_REQUIRE_EQUAL(2, tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", tags[0].name);
    BOOST_REQUIRE_EQUAL(1, tags[0].threadCount);

    BOOST_REQUIRE_EQUAL(tag2Id, tags[1].id);
    BOOST_REQUIRE_EQUAL("Tag2", tags[1].name);
    BOOST_REQUIRE_EQUAL(1, tags[1].threadCount);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                             .get_child("threads"));
    BOOST_REQUIRE_EQUAL(1, threads.size());
    BOOST_REQUIRE_EQUAL(thread2Id, threads[0].id);
    BOOST_REQUIRE_EQUAL("Thread2", threads[0].name);
    BOOST_REQUIRE_EQUAL(2, threads[0].tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, threads[0].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", threads[0].tags[0].name);
    BOOST_REQUIRE_EQUAL(tag2Id, threads[0].tags[1].id);
    BOOST_REQUIRE_EQUAL("Tag2", threads[0].tags[1].name);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_one_tag_can_be_retrieved_sorted_by_various_criteria )
{
    auto handler = createCommandHandler();
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    std::string thread1Id, thread2Id, thread3Id;
    {
        TimestampChanger _(1000);
        thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                           { tagId, thread1Id }));
        for (size_t i = 0; i < 3; i++)
        {
            createDiscussionMessageAndGetId(handler, thread1Id, "Sample");
        }
    }
    {
        TimestampChanger _(3000);
        thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                           { tagId, thread2Id }));
        for (size_t i = 0; i < 1; i++)
        {
            createDiscussionMessageAndGetId(handler, thread2Id, "Sample");
        }
    }
    {
        TimestampChanger _(2000);
        thread3Id = createDiscussionThreadAndGetId(handler, "Thread3");
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                           { tagId, thread3Id }));
        for (size_t i = 0; i < 2; i++)
        {
            createDiscussionMessageAndGetId(handler, thread3Id, "Sample");
        }
    }

    std::string ids[][3] =
    {
        { thread1Id, thread2Id, thread3Id }, //by name, ascending
        { thread3Id, thread2Id, thread1Id }, //by name, descending
        { thread1Id, thread3Id, thread2Id }, //by created, ascending
        { thread2Id, thread3Id, thread1Id }, //by created, descending
        { thread1Id, thread3Id, thread2Id }, //by last updated, ascending
        { thread2Id, thread3Id, thread1Id }, //by last updated, descending
        { thread2Id, thread3Id, thread1Id }, //by message count, ascending
        { thread1Id, thread3Id, thread2Id }, //by message count, descending
    };
    int64_t messagesCount[][3] =
    {
        { 3, 1, 2 }, //by name, ascending
        { 2, 1, 3 }, //by name, descending
        { 3, 2, 1 }, //by created, ascending
        { 1, 2, 3 }, //by created, descending
        { 3, 2, 1 }, //by last updated, ascending
        { 1, 2, 3 }, //by last updated, descending
        { 1, 2, 3 }, //by message count, ascending
        { 3, 2, 1 }, //by message count, descending
    };

    int index = 0;
    for (auto command : GetDiscussionThreadWithTagCommands)
        for (auto sortOrder : { SortOrder::Ascending, SortOrder::Descending })
        {
            auto threads = deserializeThreads(handlerToObj(handler, command, sortOrder, { tagId })
                                                     .get_child("threads"));
            BOOST_REQUIRE_EQUAL(3, threads.size());
            for (size_t i = 0; i < 3; i++)
            {
                BOOST_REQUIRE_EQUAL(ids[index][i], threads[i].id);
                BOOST_REQUIRE_EQUAL(messagesCount[index][i], threads[i].messageCount);
            }
            ++index;
        }
}

BOOST_AUTO_TEST_CASE( Listing_discussion_threads_attached_to_tags_does_not_include_messages )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tagId, threadId }));

    for (auto& item : handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME).get_child("threads"))
    {
        BOOST_REQUIRE( ! treeContains(item.second, "messages"));
    }
}

BOOST_AUTO_TEST_CASE( Merging_discussion_tags_requires_two_different_valid_tag_ids )
{
    auto handler = createCommandHandler();
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                                       { "bogus id 1", "bogus id 2" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                                       { "bogus id 1", tagId }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                                       { tagId, "bogus id 2" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, 
                                                              Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                              { sampleValidIdString, sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, 
                                                              Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                              { sampleValidIdString, tagId }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, 
                                                              Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                              { tagId, sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, 
                                                                       Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                                       { tagId, tagId }));
}

BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_merged_keeping_all_discussion_thread_references )
{
    auto handler = createCommandHandler();
    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    auto thread3Id = createDiscussionThreadAndGetId(handler, "Thread3");
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tag1Id, thread1Id }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tag1Id, thread2Id }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tag2Id, thread2Id }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tag2Id, thread3Id }));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                       { tag2Id, tag1Id }));

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));

    BOOST_REQUIRE_EQUAL(1, tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", tags[0].name);

    auto threads = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME)
                                   .get_child("threads"));

    BOOST_REQUIRE_EQUAL(3, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
    BOOST_REQUIRE_EQUAL(thread3Id, threads[2].id);
}

//deferred for a later release
//BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_multiple_tags_can_be_distinctly_retrieved_sorted_by_various_criteria ) {}
//BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_multiple_tags_can_be_filtered_by_excluded_by_tag ) {}
