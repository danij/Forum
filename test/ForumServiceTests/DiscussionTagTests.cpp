#include <vector>

#include "CommandsCommon.h"
#include "EntityCollection.h"
#include "RandomGenerator.h"
#include "TestHelpers.h"

using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

BOOST_AUTO_TEST_CASE( No_discussion_tags_are_present_before_one_is_created ) {}
BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_succeeds ) {}
BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_a_name_that_is_too_short_fails ) {}
BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_with_a_name_that_is_too_long_fails ) {}
BOOST_AUTO_TEST_CASE( Creating_multiple_discussion_tags_with_the_same_name_case_insensitive_fails ) {}
BOOST_AUTO_TEST_CASE( Approved_discussion_tags_can_be_retrieved_sorted_by_name ) {}
BOOST_AUTO_TEST_CASE( Approved_discussion_tags_can_be_retrieved_sorted_by_discussion_thread_count_ascending_and_descending ) {}
BOOST_AUTO_TEST_CASE( Approved_discussion_tags_display_the_count_of_attached_threads ) {}
BOOST_AUTO_TEST_CASE( Discussion_tags_that_require_review_can_be_retrieved_sorted_by_name ) {}
BOOST_AUTO_TEST_CASE( Renaming_a_discussion_tag_succeeds_only_if_creation_criteria_are_met ) {}
BOOST_AUTO_TEST_CASE( Deleting_an_inexisting_discussion_tag_fails ) {}
BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_succeeds ) {}

BOOST_AUTO_TEST_CASE( Discussion_threads_have_no_tags_attached_by_default ) {}
BOOST_AUTO_TEST_CASE( Existing_discussion_tags_can_be_automatically_attached_to_threads ) {}
BOOST_AUTO_TEST_CASE( Attaching_non_existing_discussion_tags_to_threads_creates_new_ones_that_require_review ) {}
BOOST_AUTO_TEST_CASE( Attaching_duplicate_discussion_tags_to_a_thread_does_not_create_duplicates ) {}
BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_merged_keeping_all_discussion_thread_references ) {}
BOOST_AUTO_TEST_CASE( Discussion_threads_list_all_attached_discussion_tags ) {}
BOOST_AUTO_TEST_CASE( Discussion_tags_can_be_detached_from_threads ) {}
BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_detaches_it_from_threads ) {}
BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_detaches_it_from_tags ) {}

BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_one_tag_can_be_retrieved_sorted_by_various_criteria ) {}
BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_multiple_tags_are_distinctly_retrieved_sorted_by_various_criteria ) {}
BOOST_AUTO_TEST_CASE( Discussion_threads_attached_to_multiple_tags_can_be_filtered_by_excluded_by_tag ) {}
BOOST_AUTO_TEST_CASE( Listing_discussion_threads_attached_to_tags_does_not_include_messages ) {}
