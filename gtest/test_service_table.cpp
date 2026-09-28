// regression: freeSubscription() must release the list nodes its event
// queue parked for reuse.
//
// The LinkedList recycles its nodes: ListDelNode() parks a node on the
// list's own free list, and only ListDestroy() frees the parked ones.
// freeSubscription() drained sub->outgoing with ListDelNode(), through
// freeSubscriptionQueuedEvents(), and never called ListDestroy(), so every
// subscription that had queued an event leaked at least one ListNode when
// it was torn down.
//
// These tests check the free list directly, so they fail without the fix
// on any build; under LeakSanitizer the lost nodes are also reported.

#include "gtest/gtest.h"

extern "C" {
#include "ThreadPool.h"
#include "service_table.h"
}

#include <cstdlib>
#include <cstring>

#if defined(INCLUDE_DEVICE_APIS) && EXCLUDE_GENA == 0

// A queued event as genaInitNotifyCommon() stores it: a heap-allocated
// ThreadPoolJob. freeSubscriptionQueuedEvents() frees the job itself, and
// looks at job->arg only for the entries after the first.
static ThreadPoolJob *new_queued_job()
{
	return static_cast<ThreadPoolJob *>(calloc(1, sizeof(ThreadPoolJob)));
}

TEST(FreeSubscription, releases_the_nodes_its_event_queue_recycled)
{
	subscription sub{};
	ASSERT_EQ(ListInit(&sub.outgoing, nullptr, free), 0);

	// Two queued events, then the head sent and removed the way
	// genaNotifyThread() does it: its node is parked, not freed.
	ListNode *head = ListAddTail(&sub.outgoing, new_queued_job());
	ASSERT_NE(head, nullptr);
	ASSERT_NE(ListAddTail(&sub.outgoing, new_queued_job()), nullptr);
	ListDelNode(&sub.outgoing, head, 1);
	ASSERT_EQ(sub.outgoing.freeNodeList.freeListLength, 1);

	freeSubscription(&sub);

	// Without the fix, both nodes are still parked here -- the sent one
	// and the one freeSubscriptionQueuedEvents() removed -- and they are
	// lost once the subscription itself is freed.
	EXPECT_EQ(sub.outgoing.freeNodeList.freeListLength, 0);
	EXPECT_EQ(sub.outgoing.freeNodeList.head, nullptr);
}

TEST(FreeSubscription, is_safe_on_a_copy_made_by_copy_subscription)
{
	// genaNotifyThread() sends each event from a stack copy of the
	// subscription, frees it with freeSubscription(), and
	// copy_subscription() gives that copy an empty event queue of its own.
	static const char callback[] = "<http://192.168.0.2:49152/cb>";
	subscription sub{};
	subscription copy{};

	ASSERT_EQ(ListInit(&sub.outgoing, nullptr, free), 0);
	sub.DeliveryURLs.URLs = strdup(callback);
	sub.DeliveryURLs.parsedURLs =
		static_cast<uri_type *>(calloc(1, sizeof(uri_type)));
	ASSERT_NE(sub.DeliveryURLs.URLs, nullptr);
	ASSERT_NE(sub.DeliveryURLs.parsedURLs, nullptr);
	ASSERT_EQ(parse_uri(sub.DeliveryURLs.URLs + 1,
			  strlen(callback) - 2,
			  &sub.DeliveryURLs.parsedURLs[0]),
		HTTP_SUCCESS);
	sub.DeliveryURLs.size = 1;
	ASSERT_NE(ListAddTail(&sub.outgoing, new_queued_job()), nullptr);

	ASSERT_EQ(copy_subscription(&sub, &copy), HTTP_SUCCESS);
	EXPECT_EQ(ListSize(&copy.outgoing), 0);

	freeSubscription(&copy);
	EXPECT_EQ(copy.outgoing.freeNodeList.freeListLength, 0);
	EXPECT_EQ(copy.outgoing.freeNodeList.head, nullptr);

	// The original is untouched by freeing the copy.
	EXPECT_EQ(ListSize(&sub.outgoing), 1);
	freeSubscription(&sub);
	EXPECT_EQ(sub.outgoing.freeNodeList.freeListLength, 0);
	EXPECT_EQ(sub.outgoing.freeNodeList.head, nullptr);
}

#endif /* INCLUDE_DEVICE_APIS && EXCLUDE_GENA == 0 */

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
