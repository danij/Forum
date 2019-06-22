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

#include "CommandsCommon.h"
#include "TestHelpers.h"

#include <vector>

using namespace Forum::Configuration;
using namespace Forum::Context;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

struct SerializedDiscussionCategoryReferencedByTagInCategoryTest
{
    std::string id;
    std::string name;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
    }
};

/**
* Stores only the information that is sent out about a discussion tag
*/
struct SerializedDiscussionTagInCategoryTest
{
    std::string id;
    std::string name;
    int64_t threadCount = 0;
    std::vector<SerializedDiscussionCategoryReferencedByTagInCategoryTest> categories;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        threadCount = tree.get<int64_t>("threadCount");

        for (auto& pair : tree)
        {
            if (pair.first == "categories")
            {
                categories = deserializeEntities<SerializedDiscussionCategoryReferencedByTagInCategoryTest>(pair.second);
            }
        }
    }
};

CREATE_FUNCTION_ALIAS(deserializeTags, deserializeEntities<SerializedDiscussionTagInCategoryTest>)

/**
* Stores only the information that is sent out about a user referenced in a discussion thread or message
*/
struct SerializedUserReferencedInDiscussionThreadOrMessageInCategoryTest
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

struct SerializedLatestDiscussionThreadMessageInCategoryTest
{
    Timestamp created = 0;
    SerializedUserReferencedInDiscussionThreadOrMessageInCategoryTest createdBy;

    void populate(const boost::property_tree::ptree& tree)
    {
        created = tree.get<Timestamp>("created");

        createdBy.populate(tree.get_child("createdBy"));
    }
};

struct SerializedDiscussionThreadInCategoryTest
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;
    SerializedUserReferencedInDiscussionThreadOrMessageInCategoryTest createdBy;
    int64_t visited = 0;
    int64_t messageCount = 0;
    SerializedLatestDiscussionThreadMessageInCategoryTest latestMessage;
    std::vector<SerializedDiscussionTagInCategoryTest> tags;

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

CREATE_FUNCTION_ALIAS(deserializeThreads, deserializeEntities<SerializedDiscussionThreadInCategoryTest>)

struct SerializedDiscussionCategoryParentReferenceInCategoryTest
{
    std::string id;
    std::string name;
    std::unique_ptr<SerializedDiscussionCategoryParentReferenceInCategoryTest> parent;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        for (auto& pair : tree)
        {
            if (pair.first == "parent")
            {
                parent = std::make_unique<SerializedDiscussionCategoryParentReferenceInCategoryTest>();
                parent->populate(pair.second);
            }
        }
    }
};

struct SerializedDiscussionCategory;

CREATE_FUNCTION_ALIAS(deserializeCategory, deserializeEntity<SerializedDiscussionCategory>)
CREATE_FUNCTION_ALIAS(deserializeCategories, deserializeEntities<SerializedDiscussionCategory>)

static bool serializedDiscussionCategoryByDisplayOrderLess(const SerializedDiscussionCategory& first,
                                                           const SerializedDiscussionCategory& second);

/**
* Stores only the information that is sent out about a discussion category
*/
struct SerializedDiscussionCategory
{
    std::string id;
    std::string name;
    std::string description;
    int displayOrder;
    int threadCount;
    int messageCount;
    int threadTotalCount;
    int messageTotalCount;
    std::unique_ptr<SerializedDiscussionCategoryParentReferenceInCategoryTest> parent;
    //Tags that are directly attached to the current category
    std::vector<SerializedDiscussionTagInCategoryTest> tags;
    std::vector<SerializedDiscussionCategory> children;
    std::unique_ptr<SerializedLatestDiscussionThreadMessageInCategoryTest> latestMessage;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        description = tree.get<std::string>("description");
        displayOrder = tree.get<int>("displayOrder");
        threadCount = tree.get<int>("threadCount");
        messageCount = tree.get<int>("messageCount");
        threadTotalCount = tree.get<int>("threadTotalCount");
        messageTotalCount = tree.get<int>("messageTotalCount");
        for (auto& pair : tree)
        {
            if (pair.first == "parent")
            {
                parent = std::make_unique<SerializedDiscussionCategoryParentReferenceInCategoryTest>();
                parent->populate(pair.second);
            }
            else if (pair.first == "tags")
            {
                tags = deserializeTags(pair.second);
            }
            else if (pair.first == "children")
            {
                children = deserializeCategories(pair.second);
                std::sort(children.begin(), children.end(), serializedDiscussionCategoryByDisplayOrderLess);
            }
            else if (pair.first == "latestMessage")
            {
                latestMessage = std::make_unique<SerializedLatestDiscussionThreadMessageInCategoryTest>();
                latestMessage->populate(pair.second);
            }
        }
    }
};

static bool serializedDiscussionCategoryByDisplayOrderLess(const SerializedDiscussionCategory& first,
                                                           const SerializedDiscussionCategory& second)
{
    return first.displayOrder < second.displayOrder;
}

auto getCategories(Forum::Commands::CommandHandlerRef handler, Forum::Commands::View view)
{
    return deserializeCategories(handlerToObj(handler, view).get_child("categories"));
}

auto getCategory(Forum::Commands::CommandHandlerRef handler, std::string id)
{
    return deserializeCategory(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORY_BY_ID, { id })
                                 .get_child("category"));
}

void deleteCategory(Forum::Commands::CommandHandlerRef handler, std::string id)
{
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_CATEGORY, { id }));
}

void addTagToCategory(Forum::Commands::CommandHandlerRef handler, std::string tagId, std::string categoryId)
{
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                       { tagId, categoryId }));
}

void removeTagFromCategory(Forum::Commands::CommandHandlerRef handler, std::string tagId, std::string categoryId)
{
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_CATEGORY,
                                                       { tagId, categoryId }));
}

void addTagToThread(Forum::Commands::CommandHandlerRef handler, std::string tagId, std::string threadId)
{
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD,
                                                       { tagId, threadId }));
}

void removeTagFromThread(Forum::Commands::CommandHandlerRef handler, std::string tagId, std::string threadId)
{
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_THREAD,
                                                       { tagId, threadId }));
}

BOOST_AUTO_TEST_CASE( No_discussion_categories_are_present_before_one_is_created )
{
    auto handler = createCommandHandler();
    auto categories = getCategories(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_NAME);

    BOOST_REQUIRE_EQUAL(0u, categories.size());

    categories = getCategories(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT);

    BOOST_REQUIRE_EQUAL(0u, categories.size());
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_returns_the_id_name_and_empty_parent_id )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "Foo" });

    assertStatusCodeEqual(StatusCode::OK, returnObject);

    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
    BOOST_REQUIRE(isIdEmpty(returnObject.get<std::string>("parentId")));
}

BOOST_AUTO_TEST_CASE( Creating_a_child_discussion_category_returns_the_id_name_and_parent_id )
{
    auto handler = createCommandHandler();
    auto parentId = createDiscussionCategoryAndGetId(handler, "Parent");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "Foo", parentId });

    assertStatusCodeEqual(StatusCode::OK, returnObject);

    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
    BOOST_REQUIRE_EQUAL(parentId, returnObject.get<std::string>("parentId"));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_no_parameters_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY);
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_empty_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_only_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_leading_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_trailing_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionCategory.minNameLength - 1, 'a');
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_a_too_long_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionCategory.maxNameLength + 1, 'a');
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_with_a_name_that_contains_invalid_characters_fails_with_appropriate_message )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "\xFF\xFF" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_multiple_discussion_categories_with_the_same_name_case_insensitive_fails )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "Foo" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_CATEGORY, { "fȏo" }));
}

BOOST_AUTO_TEST_CASE( Renaming_a_discussion_category_succeeds_only_if_creation_criteria_are_met )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Foo");

    auto config = getGlobalConfig();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, "" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, " \t\r\n" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, " Foo" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, "Foo\t" }));
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME,
                                       { categoryId, std::string(config->discussionCategory.minNameLength - 1, 'a') }));
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME,
                                       { categoryId, std::string(config->discussionCategory.maxNameLength + 1, 'a') }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, "\xFF\xFF" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, "fȏo" }));
}

BOOST_AUTO_TEST_CASE( Changing_the_display_order_of_discussion_categories_requires_valid_integer_inputs )
{
    auto handler = createCommandHandler();

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Parent");

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                      { categoryId, "abcd" }));
}

BOOST_AUTO_TEST_CASE( Discussion_categories_are_ordered_ascending_by_their_display_order_relative_to_their_parent )
{
    auto handler = createCommandHandler();

    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategory1Id = createDiscussionCategoryAndGetId(handler, "Child1-200", parentCategoryId);
    auto childCategory2Id = createDiscussionCategoryAndGetId(handler, "Child2-100", parentCategoryId);
    auto childCategory3Id = createDiscussionCategoryAndGetId(handler, "Child3-300", parentCategoryId);

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory1Id, "200" }));
    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory2Id, "100" }));
    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory3Id, "300" }));

    auto category = getCategory(handler, parentCategoryId);
    BOOST_REQUIRE_EQUAL(parentCategoryId, category.id);
    BOOST_REQUIRE_EQUAL("Parent", category.name);
    BOOST_REQUIRE_EQUAL(3u, category.children.size());

    BOOST_REQUIRE_EQUAL(childCategory2Id, category.children[0].id);
    BOOST_REQUIRE_EQUAL("Child2-100", category.children[0].name);
    BOOST_REQUIRE_EQUAL(100, category.children[0].displayOrder);

    BOOST_REQUIRE_EQUAL(childCategory1Id, category.children[1].id);
    BOOST_REQUIRE_EQUAL("Child1-200", category.children[1].name);
    BOOST_REQUIRE_EQUAL(200, category.children[1].displayOrder);

    BOOST_REQUIRE_EQUAL(childCategory3Id, category.children[2].id);
    BOOST_REQUIRE_EQUAL("Child3-300", category.children[2].name);
    BOOST_REQUIRE_EQUAL(300, category.children[2].displayOrder);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_parent_works )
{
    auto handler = createCommandHandler();
    auto parentCategory1Id = createDiscussionCategoryAndGetId(handler, "Parent1");
    auto parentCategory2Id = createDiscussionCategoryAndGetId(handler, "Parent2");

    auto childCategory1Id = createDiscussionCategoryAndGetId(handler, "Child1-200", parentCategory1Id);
    auto childCategory2Id = createDiscussionCategoryAndGetId(handler, "Child2-100", parentCategory2Id);
    auto childCategory3Id = createDiscussionCategoryAndGetId(handler, "Child3-300", parentCategory2Id);
    auto childCategory4Id = createDiscussionCategoryAndGetId(handler, "Child4", parentCategory2Id);

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory1Id, "200" }));
    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory2Id, "100" }));
    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory3Id, "300" }));
    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                       { childCategory4Id, "400" }));

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT,
                                       { childCategory1Id, parentCategory2Id }));

    auto category1 = getCategory(handler, parentCategory1Id);
    BOOST_REQUIRE_EQUAL(parentCategory1Id, category1.id);
    BOOST_REQUIRE_EQUAL("Parent1", category1.name);
    BOOST_REQUIRE_EQUAL(0u, category1.children.size());

    auto category2 = getCategory(handler, parentCategory2Id);
    BOOST_REQUIRE_EQUAL(parentCategory2Id, category2.id);
    BOOST_REQUIRE_EQUAL("Parent2", category2.name);
    BOOST_REQUIRE_EQUAL(4u, category2.children.size());

    BOOST_REQUIRE_EQUAL(childCategory2Id, category2.children[0].id);
    BOOST_REQUIRE_EQUAL("Child2-100", category2.children[0].name);
    BOOST_REQUIRE_EQUAL(100, category2.children[0].displayOrder);

    BOOST_REQUIRE_EQUAL(childCategory1Id, category2.children[1].id);
    BOOST_REQUIRE_EQUAL("Child1-200", category2.children[1].name);
    BOOST_REQUIRE_EQUAL(200, category2.children[1].displayOrder);

    BOOST_REQUIRE_EQUAL(childCategory3Id, category2.children[2].id);
    BOOST_REQUIRE_EQUAL("Child3-300", category2.children[2].name);
    BOOST_REQUIRE_EQUAL(300, category2.children[2].displayOrder);

    BOOST_REQUIRE_EQUAL(childCategory4Id, category2.children[3].id);
    BOOST_REQUIRE_EQUAL("Child4", category2.children[3].name);
    BOOST_REQUIRE_EQUAL(400, category2.children[3].displayOrder);

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT,
                                       { childCategory4Id, sampleValidIdString }));

    auto childCategory4 = getCategory(handler, childCategory4Id);
    BOOST_REQUIRE_EQUAL(childCategory4Id, childCategory4.id);
    BOOST_REQUIRE_EQUAL("Child4", childCategory4.name);
    BOOST_REQUIRE( ! childCategory4.parent);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_parent_fails_on_circular_links )
{
    auto handler = createCommandHandler();
    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);
    auto childChildCategoryId = createDiscussionCategoryAndGetId(handler, "ChildChild", childCategoryId);

    assertStatusCodeEqual(StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT,
                                       { parentCategoryId, childCategoryId }));
    assertStatusCodeEqual(StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT,
                                       { parentCategoryId, childChildCategoryId }));
    assertStatusCodeEqual(StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT,
                                       { childCategoryId, childChildCategoryId }));
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_category_with_an_invalid_id_returns_invalid_parameters )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_CATEGORY, { "bogus id" }));
}

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_discussion_category_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler,
                                                              Forum::Commands::DELETE_DISCUSSION_CATEGORY,
                                                              { sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Deleted_discussion_categories_can_no_longer_be_retrieved )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };
    std::vector<std::string> ids;

    for (auto& name : names)
    {
        ids.push_back(createDiscussionCategoryAndGetId(handler, name));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionCategories"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_CATEGORY, { ids[0] }));

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionCategories"));

    auto categories = getCategories(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_NAME);

    BOOST_REQUIRE_EQUAL(names.size() - 1, categories.size());
    BOOST_REQUIRE_EQUAL("Def", categories[0].name);
    BOOST_REQUIRE_EQUAL("Ghi", categories[1].name);
}

BOOST_AUTO_TEST_CASE( Deleting_discussion_categories_moves_child_categories_to_root )
{
    auto handler = createCommandHandler();

    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_CATEGORY,
                                                       { parentCategoryId }));

    auto categories = getCategories(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_NAME);

    BOOST_REQUIRE_EQUAL(1u, categories.size());
    BOOST_REQUIRE_EQUAL(childCategoryId, categories[0].id);
    BOOST_REQUIRE_EQUAL("Child", categories[0].name);
    BOOST_REQUIRE( ! categories[0].parent);
}

BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_attached_to_categories_even_if_they_are_already_attached )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                       { tagId, categoryId }));

    assertStatusCodeEqual(StatusCode::NO_EFFECT, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                              { tagId, categoryId }));

    auto category = getCategory(handler, categoryId);

    BOOST_REQUIRE_EQUAL(categoryId, category.id);
    BOOST_REQUIRE_EQUAL("Category", category.name);
    BOOST_REQUIRE_EQUAL(1u, category.tags.size());
    BOOST_REQUIRE_EQUAL(tagId, category.tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag", category.tags[0].name);
}

BOOST_AUTO_TEST_CASE( Attaching_discussion_tags_require_a_valid_discussion_tag_and_a_valid_category )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler,
                                                                       Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                                       { "bogus tag id", "bogus category id" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler,
                                                                       Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                                       { tagId, "bogus category id" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler,
                                                                       Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                                       { "bogus tag id", categoryId }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler,
                                                              Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                              { sampleValidIdString, sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler,
                                                              Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                              { tagId, sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler,
                                                              Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                              { sampleValidIdString, categoryId }));
}

BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_detached_from_categories )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                       { tagId, categoryId }));

    auto category = getCategory(handler, categoryId);
    BOOST_REQUIRE_EQUAL(1u, category.tags.size());

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_CATEGORY,
                                                       { tagId, categoryId }));

    category = getCategory(handler, categoryId);
    BOOST_REQUIRE_EQUAL(0u, category.tags.size());
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_detaches_it_from_categories )
{
    auto handler = createCommandHandler();
    auto category1Id = createDiscussionCategoryAndGetId(handler, "Category1");
    auto category2Id = createDiscussionCategoryAndGetId(handler, "Category2");
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    for (auto& categoryId : { category1Id, category2Id })
        for (auto& tagId : { tag1Id, tag2Id })
        {
            assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                               { tagId, categoryId }));
        }
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_TAG, { tag1Id }));

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));
    BOOST_REQUIRE_EQUAL(1u, tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", tags[0].name);
    BOOST_REQUIRE_EQUAL(2u, tags[0].categories.size());

    auto categories = getCategories(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_NAME);

    BOOST_REQUIRE_EQUAL(2u, categories.size());
    BOOST_REQUIRE_EQUAL(category1Id, categories[0].id);
    BOOST_REQUIRE_EQUAL("Category1", categories[0].name);
    BOOST_REQUIRE_EQUAL(1u, categories[0].tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, categories[0].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", categories[0].tags[0].name);

    BOOST_REQUIRE_EQUAL(category2Id, categories[1].id);
    BOOST_REQUIRE_EQUAL("Category2", categories[1].name);
    BOOST_REQUIRE_EQUAL(1u, categories[1].tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, categories[1].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", categories[1].tags[0].name);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_category_detaches_it_from_tags )
{
    auto handler = createCommandHandler();
    auto category1Id = createDiscussionCategoryAndGetId(handler, "Category1");
    auto category2Id = createDiscussionCategoryAndGetId(handler, "Category2");
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    for (auto& threadId : { category1Id, category2Id })
        for (auto& tagId : { tag1Id, tag2Id })
        {
            assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY,
                                                               { tagId, threadId }));
        }

    deleteCategory(handler, category1Id);

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));
    BOOST_REQUIRE_EQUAL(2u, tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", tags[0].name);
    BOOST_REQUIRE_EQUAL(1u, tags[0].categories.size());
    BOOST_REQUIRE_EQUAL(category2Id, tags[0].categories[0].id);
    BOOST_REQUIRE_EQUAL("Category2", tags[0].categories[0].name);

    BOOST_REQUIRE_EQUAL(tag2Id, tags[1].id);
    BOOST_REQUIRE_EQUAL("Tag2", tags[1].name);
    BOOST_REQUIRE_EQUAL(1u, tags[1].categories.size());
    BOOST_REQUIRE_EQUAL(category2Id, tags[1].categories[0].id);
    BOOST_REQUIRE_EQUAL("Category2", tags[1].categories[0].name);
}

BOOST_AUTO_TEST_CASE( Discussion_categories_include_two_levels_of_children_in_results )
{
    auto handler = createCommandHandler();
    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);
    auto childChildCategoryId = createDiscussionCategoryAndGetId(handler, "ChildChild", childCategoryId);
    auto childChildChildCategoryId = createDiscussionCategoryAndGetId(handler, "ChildChildChild", childChildCategoryId);

    auto parentCategory = getCategory(handler, parentCategoryId);

    BOOST_REQUIRE_EQUAL(parentCategoryId, parentCategory.id);
    BOOST_REQUIRE_EQUAL(1u, parentCategory.children.size());

    BOOST_REQUIRE_EQUAL(childCategoryId, parentCategory.children[0].id);
    BOOST_REQUIRE_EQUAL(1u, parentCategory.children[0].children.size());

    BOOST_REQUIRE_EQUAL(childChildCategoryId, parentCategory.children[0].children[0].id);
    BOOST_REQUIRE_EQUAL(0u, parentCategory.children[0].children[0].children.size());
}

BOOST_AUTO_TEST_CASE( Discussion_categories_include_all_parent_levels_in_results )
{
    auto handler = createCommandHandler();
    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);
    auto childChildCategoryId = createDiscussionCategoryAndGetId(handler, "ChildChild", childCategoryId);

    auto childChildCategory = getCategory(handler, childChildCategoryId);

    BOOST_REQUIRE_EQUAL(childChildCategoryId, childChildCategory.id);
    BOOST_REQUIRE_EQUAL("ChildChild", childChildCategory.name);

    BOOST_REQUIRE_EQUAL(0u, childChildCategory.children.size());

    BOOST_REQUIRE(childChildCategory.parent);
    BOOST_REQUIRE_EQUAL(childCategoryId, childChildCategory.parent->id);
    BOOST_REQUIRE_EQUAL("Child", childChildCategory.parent->name);

    BOOST_REQUIRE(childChildCategory.parent->parent);
    BOOST_REQUIRE_EQUAL(parentCategoryId, childChildCategory.parent->parent->id);
    BOOST_REQUIRE_EQUAL("Parent", childChildCategory.parent->parent->name);

    BOOST_REQUIRE( ! childChildCategory.parent->parent->parent);
}

BOOST_AUTO_TEST_CASE( Discussion_categories_include_tags_of_current_and_one_level_of_children_in_results )
{
    auto handler = createCommandHandler();
    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    addTagToCategory(handler, tag1Id, parentCategoryId);
    addTagToCategory(handler, tag2Id, childCategoryId);

    auto parentCategory = getCategory(handler, parentCategoryId);

    BOOST_REQUIRE_EQUAL(parentCategoryId, parentCategory.id);
    BOOST_REQUIRE_EQUAL(1u, parentCategory.tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, parentCategory.tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", parentCategory.tags[0].name);

    BOOST_REQUIRE_EQUAL(1u, parentCategory.children.size());
    BOOST_REQUIRE_EQUAL(childCategoryId, parentCategory.children[0].id);
    BOOST_REQUIRE_EQUAL(1u, parentCategory.children[0].tags.size());
    BOOST_REQUIRE_EQUAL(tag2Id, parentCategory.children[0].tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag2", parentCategory.children[0].tags[0].name);
}

BOOST_AUTO_TEST_CASE( Discussion_categories_recursively_include_total_thread_and_message_count )
{
    auto handler = createCommandHandler();

    LoggedInUserChanger __(createUserAndGetId(handler, "User"));

    auto category1Id = createDiscussionCategoryAndGetId(handler, "Category1");
    auto childCategory1Id = createDiscussionCategoryAndGetId(handler, "ChildCategory1", category1Id);
    auto category2Id = createDiscussionCategoryAndGetId(handler, "Category2");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                       Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                                       { category1Id, "1" }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                       Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
                                                       { category2Id, "2" }));

    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag11Id = createDiscussionTagAndGetId(handler, "Tag11");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    addTagToCategory(handler, tag1Id, category1Id);
    addTagToCategory(handler, tag11Id, childCategory1Id);
    addTagToCategory(handler, tag2Id, category2Id);

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1 only on Category1");
    addTagToThread(handler, tag1Id, thread1Id);
    for (size_t i = 0; i < 10; i++)
    {
        createDiscussionMessageAndGetId(handler, thread1Id, "Message for Thread1");
    }

    auto thread11BothId = createDiscussionThreadAndGetId(handler, "Thread11 on Category1 and ChildCategory1");
    addTagToThread(handler, tag1Id, thread11BothId);//although added to tag1, should not be counted twice on category 1
    addTagToThread(handler, tag11Id, thread11BothId);
    for (size_t i = 0; i < 5; i++)
    {
        createDiscussionMessageAndGetId(handler, thread11BothId, "Message for Thread11 Both");
    }

    auto thread11OnlyId = createDiscussionThreadAndGetId(handler, "Thread11 only on ChildCategory1");
    addTagToThread(handler, tag11Id, thread11OnlyId);
    for (size_t i = 0; i < 3; i++)
    {
        createDiscussionMessageAndGetId(handler, thread11OnlyId, "Message for Thread11 Only");
    }

    auto thread2OnlyId = createDiscussionThreadAndGetId(handler, "Thread2 only on Category2");
    addTagToThread(handler, tag2Id, thread2OnlyId);
    for (size_t i = 0; i < 7; i++)
    {
        createDiscussionMessageAndGetId(handler, thread2OnlyId, "Message for Thread2 Only");
    }

    auto thread21BothId = createDiscussionThreadAndGetId(handler, "Thread21 on Category2 and Category1");
    addTagToThread(handler, tag1Id, thread21BothId);
    addTagToThread(handler, tag2Id, thread21BothId);
    for (size_t i = 0; i < 20; i++)
    {
        createDiscussionMessageAndGetId(handler, thread21BothId, "Message for Thread21 Both");
    }

    auto categories = getCategories(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_FROM_ROOT);

    BOOST_REQUIRE_EQUAL(2u, categories.size());
    BOOST_REQUIRE_EQUAL(category1Id, categories[0].id);
    BOOST_REQUIRE_EQUAL(4, categories[0].threadTotalCount);
    BOOST_REQUIRE_EQUAL(38, categories[0].messageTotalCount);

    BOOST_REQUIRE_EQUAL(1u, categories[0].children.size());
    BOOST_REQUIRE_EQUAL(childCategory1Id, categories[0].children[0].id);
    BOOST_REQUIRE_EQUAL(2, categories[0].children[0].threadTotalCount);
    BOOST_REQUIRE_EQUAL(8, categories[0].children[0].messageTotalCount);

    BOOST_REQUIRE_EQUAL(category2Id, categories[1].id);
    BOOST_REQUIRE_EQUAL(2, categories[1].threadTotalCount);
    BOOST_REQUIRE_EQUAL(27, categories[1].messageTotalCount);
}

static Forum::Commands::View GetDiscussionThreadsOfCategoryViews[] =
{
    Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
    Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED,
    Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED,
    Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT,
};

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_an_invalid_category_fails )
{
    auto handler = createCommandHandler();
    for (auto command : GetDiscussionThreadsOfCategoryViews)
    {
        assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, command, { "bogus id" }));
    }
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_an_unknown_category_returns_not_found )
{
    auto handler = createCommandHandler();
    for (auto command : GetDiscussionThreadsOfCategoryViews)
    {
        assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, command, { sampleValidIdString }));
    }
}

BOOST_AUTO_TEST_CASE( Discussion_categories_have_no_threads_attached_by_default )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Foo");

    for (auto command : GetDiscussionThreadsOfCategoryViews)
    {
        auto threads = deserializeThreads(handlerToObj(handler, command, { categoryId }).get_child("threads"));
        BOOST_REQUIRE_EQUAL(0u, threads.size());
    }
}

BOOST_AUTO_TEST_CASE( Discussion_categories_include_latest_message_of_latest_thread_of_child_categories )
{
    auto handler = createCommandHandler();

    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    addTagToCategory(handler, tagId, childCategoryId);

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");

    std::string thread1Id, message1Id, message2Id;
    {
        TimestampChanger _(1000);
        LoggedInUserChanger __(user1Id);
        thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
        addTagToThread(handler, tagId, thread1Id);

        createDiscussionMessageAndGetId(handler, thread1Id, "Message 1");
    }
    {
        TimestampChanger _(2000);
        LoggedInUserChanger __(user2Id);

        createDiscussionMessageAndGetId(handler, thread1Id, "Message 2");
    }

    auto category = getCategory(handler, parentCategoryId);

    BOOST_REQUIRE_EQUAL(parentCategoryId, category.id);
    BOOST_REQUIRE_EQUAL(1u, category.children.size());
    BOOST_REQUIRE(category.latestMessage);
    BOOST_REQUIRE_EQUAL(2000u, category.latestMessage->created);
    BOOST_REQUIRE_EQUAL(user2Id, category.latestMessage->createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", category.latestMessage->createdBy.name);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_one_category_can_be_retrieved_sorted_by_various_criteria )
{
    auto handler = createCommandHandler();

    LoggedInUserChanger __(createUserAndGetId(handler, "User"));

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    addTagToCategory(handler, tagId, categoryId);

    std::string thread1Id, thread2Id, thread3Id;
    {
        TimestampChanger _(1000);
        thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
        addTagToThread(handler, tagId, thread1Id);
        for (size_t i = 0; i < 3; i++)
        {
            createDiscussionMessageAndGetId(handler, thread1Id, "Sample");
        }
    }
    {
        TimestampChanger _(3000);
        thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
        addTagToThread(handler, tagId, thread2Id);
        for (size_t i = 0; i < 1; i++)
        {
            createDiscussionMessageAndGetId(handler, thread2Id, "Sample");
        }
    }
    {
        TimestampChanger _(2000);
        thread3Id = createDiscussionThreadAndGetId(handler, "Thread3");
        addTagToThread(handler, tagId, thread3Id);
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
    for (auto command : GetDiscussionThreadsOfCategoryViews)
        for (auto sortOrder : { SortOrder::Ascending, SortOrder::Descending })
        {
            auto threads = deserializeThreads(handlerToObj(handler, command, sortOrder, { categoryId })
                                                     .get_child("threads"));
            BOOST_REQUIRE_EQUAL(3u, threads.size());
            for (size_t i = 0; i < 3; i++)
            {
                BOOST_REQUIRE_EQUAL(ids[index][i], threads[i].id);
                BOOST_REQUIRE_EQUAL(messagesCount[index][i], threads[i].messageCount);
            }
            ++index;
        }
}

BOOST_AUTO_TEST_CASE( Listing_discussion_threads_attached_to_categories_does_not_include_messages )
{
    auto handler = createCommandHandler();

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");
    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

    addTagToCategory(handler, tagId, categoryId);
    addTagToThread(handler, tagId, threadId);

    auto threads = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME).get_child("threads");
    for (auto& item : threads)
    {
        BOOST_REQUIRE( ! treeContains(item.second, "messages"));
    }
}

BOOST_AUTO_TEST_CASE( Detaching_a_discussion_tag_from_a_thread_removes_it_from_category_if_not_linked_by_other_tags )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");

    addTagToCategory(handler, tag1Id, categoryId);
    addTagToCategory(handler, tag2Id, categoryId);

    addTagToThread(handler, tag1Id, thread1Id);
    addTagToThread(handler, tag1Id, thread2Id);
    addTagToThread(handler, tag2Id, thread2Id);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                                   { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);

    removeTagFromThread(handler, tag1Id, thread1Id);
    removeTagFromThread(handler, tag1Id, thread2Id);

    threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                              { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1u, threads.size());
    BOOST_REQUIRE_EQUAL(thread2Id, threads[0].id);
}


BOOST_AUTO_TEST_CASE( Detaching_a_discussion_tag_from_a_category_removes_threads_linked_to_tag_from_category_if_not_linked_by_other_tags )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");

    addTagToCategory(handler, tag1Id, categoryId);
    addTagToCategory(handler, tag2Id, categoryId);

    addTagToThread(handler, tag1Id, thread1Id);
    addTagToThread(handler, tag1Id, thread2Id);
    addTagToThread(handler, tag2Id, thread2Id);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                                   { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);

    removeTagFromCategory(handler, tag1Id, categoryId);

    threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                              { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1u, threads.size());
    BOOST_REQUIRE_EQUAL(thread2Id, threads[0].id);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_from_a_thread_removes_it_from_category_if_not_linked_by_other_tags )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");

    addTagToCategory(handler, tag1Id, categoryId);
    addTagToCategory(handler, tag2Id, categoryId);

    addTagToThread(handler, tag1Id, thread1Id);
    addTagToThread(handler, tag1Id, thread2Id);
    addTagToThread(handler, tag2Id, thread2Id);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                                   { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);

    deleteDiscussionTag(handler, tag1Id);

    threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                              { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1u, threads.size());
    BOOST_REQUIRE_EQUAL(thread2Id, threads[0].id);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_removes_it_from_a_category )
{
    auto handler = createCommandHandler();
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");

    addTagToCategory(handler, tag1Id, categoryId);
    addTagToCategory(handler, tag2Id, categoryId);

    addTagToThread(handler, tag1Id, thread1Id);
    addTagToThread(handler, tag1Id, thread2Id);
    addTagToThread(handler, tag2Id, thread2Id);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                                   { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);

    deleteDiscussionThread(handler, thread1Id);

    threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                              { categoryId }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1u, threads.size());
    BOOST_REQUIRE_EQUAL(thread2Id, threads[0].id);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_updates_latest_message_of_each_category_it_is_part_of )
{
    auto handler = createCommandHandler();

    auto parentCategoryId = createDiscussionCategoryAndGetId(handler, "Parent");
    auto childCategoryId = createDiscussionCategoryAndGetId(handler, "Child", parentCategoryId);

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    addTagToCategory(handler, tagId, childCategoryId);

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");

    std::string thread1Id, thread2Id, message1Id, message2Id;
    {
        TimestampChanger _(1000);
        LoggedInUserChanger __(user1Id);
        thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
        addTagToThread(handler, tagId, thread1Id);

        createDiscussionMessageAndGetId(handler, thread1Id, "Message 1");
    }
    {
        TimestampChanger _(2000);
        LoggedInUserChanger __(user2Id);

        thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
        addTagToThread(handler, tagId, thread2Id);
        createDiscussionMessageAndGetId(handler, thread2Id, "Message 2");
    }

    deleteDiscussionThread(handler, thread2Id);

    auto category = getCategory(handler, parentCategoryId);

    BOOST_REQUIRE_EQUAL(parentCategoryId, category.id);
    BOOST_REQUIRE_EQUAL(1u, category.children.size());
    BOOST_REQUIRE(category.latestMessage);
    BOOST_REQUIRE_EQUAL(1000u, category.latestMessage->created);
    BOOST_REQUIRE_EQUAL(user1Id, category.latestMessage->createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", category.latestMessage->createdBy.name);
}

BOOST_AUTO_TEST_CASE( Merging_discussion_tags_updates_threads_in_categories )
{
    auto handler = createCommandHandler();

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");
    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    auto thread3Id = createDiscussionThreadAndGetId(handler, "Thread3");
    auto tag1Id = createDiscussionTagAndGetId(handler, "Tag1");
    auto tag2Id = createDiscussionTagAndGetId(handler, "Tag2");

    addTagToCategory(handler, tag1Id, categoryId);

    addTagToThread(handler, tag1Id, thread1Id);
    addTagToThread(handler, tag1Id, thread2Id);
    addTagToThread(handler, tag2Id, thread2Id);
    addTagToThread(handler, tag2Id, thread3Id);

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                                   { categoryId }).get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,
                                                       { tag2Id, tag1Id }));

    auto tags = deserializeTags(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME).get_child("tags"));

    BOOST_REQUIRE_EQUAL(1u, tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", tags[0].name);

    auto category = getCategory(handler, categoryId);
    BOOST_REQUIRE_EQUAL(1u, category.tags.size());
    BOOST_REQUIRE_EQUAL(tag1Id, category.tags[0].id);
    BOOST_REQUIRE_EQUAL("Tag1", category.tags[0].name);

    threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
                                              { categoryId }).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, threads.size());
    BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
    BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
    BOOST_REQUIRE_EQUAL(thread3Id, threads[2].id);
}
