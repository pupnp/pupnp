// GHSA-mrw2-788c-f6w7: ssdp_handle_ctrlpt_msg() queued send_search_result()
// on gRecvThreadPool with a copy of the search cookie, and the job called the
// application back without checking that the client handle was still
// registered. UpnpUnRegisterClient() promises that no callback is made after
// it returns, but a job queued before it ran afterwards with a cookie the
// application may already have freed.
//
// searchExpired() had the same problem: results queued before the search
// timed out were delivered after UPNP_DISCOVERY_SEARCH_TIMEOUT, when the
// application may also have freed the cookie.
//
// The tests fill every gRecvThreadPool worker with a job that waits on a
// gate, so the search result job stays queued while the search ends.
//
// Advertisements (ssdp:alive, ssdp:byebye) are not queued, but their callback
// ran after the handle lock was released, so UpnpUnRegisterClient() could
// return while it was still running. The UPNP_DISCOVERY_SEARCH_TIMEOUT
// callback of searchExpired() had the same problem.

#include "upnp.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

extern "C" {
#include "ThreadPool.h"
#include "config.h"
#include "httpparser.h"
#include "ssdplib.h"
#include "upnpapi.h"
}

#include <arpa/inet.h>
#include <netinet/in.h>

namespace
{

// Set once the search ended: the client unregistered or the search timed out.
std::atomic<bool> g_ended{false};
std::atomic<bool> g_timed_out{false};
std::atomic<int> g_late_results{0};

int ctrlpt_callback(Upnp_EventType type, void *, void *)
{
	if (type == UPNP_DISCOVERY_SEARCH_TIMEOUT) {
		g_timed_out = true;
		g_ended = true;
	} else if (type == UPNP_DISCOVERY_SEARCH_RESULT && g_ended) {
		g_late_results++;
	}
	return 0;
}

void ResetEvents()
{
	g_ended = false;
	g_timed_out = false;
	g_late_results = 0;
}

// Holds every gRecvThreadPool worker until Release().
class WorkerGate
{
public:
	// Adds one waiting job per worker and returns once all of them run.
	bool Fill(ThreadPool *tp, ThreadPriority priority)
	{
		ThreadPoolAttr attr;
		if (ThreadPoolGetAttr(tp, &attr) != 0)
			return false;
		const int n = attr.maxThreads;
		for (int i = 0; i < n; i++) {
			ThreadPoolJob job;
			memset(&job, 0, sizeof(job));
			TPJobInit(&job, Wait, this);
			TPJobSetPriority(&job, priority);
			TPJobSetFreeFunction(&job, nullptr);
			if (ThreadPoolAdd(tp, &job, nullptr) != 0)
				return false;
		}
		std::unique_lock<std::mutex> lk(m_);
		return cv_.wait_for(lk, std::chrono::seconds(10), [&] {
			return running_ == n;
		});
	}

	// Lets the jobs finish and waits for them to leave.
	void Release()
	{
		std::unique_lock<std::mutex> lk(m_);
		open_ = true;
		cv_.notify_all();
		cv_.wait_for(lk, std::chrono::seconds(10), [&] {
			return running_ == 0;
		});
	}

private:
	static void Wait(void *arg)
	{
		auto *self = static_cast<WorkerGate *>(arg);
		std::unique_lock<std::mutex> lk(self->m_);
		self->running_++;
		self->cv_.notify_all();
		self->cv_.wait(lk, [&] { return self->open_; });
		self->running_--;
		self->cv_.notify_all();
	}

	std::mutex m_;
	std::condition_variable cv_;
	int running_{0};
	bool open_{false};
};

// Parses a search response the way readFromSSDPSocket() does and hands it to
// the control point.
void DeliverSearchResponse()
{
	static const char resp[] = "HTTP/1.1 200 OK\r\n"
				   "CACHE-CONTROL: max-age=1800\r\n"
				   "EXT:\r\n"
				   "LOCATION: http://127.0.0.1:1/desc.xml\r\n"
				   "SERVER: test/1.0 UPnP/1.0\r\n"
				   "ST: ssdp:all\r\n"
				   "USN: uuid:test::upnp:rootdevice\r\n"
				   "\r\n";
	http_parser_t parser;
	parser_response_init(&parser, HTTPMETHOD_MSEARCH);
	ASSERT_EQ(
		parser_append(&parser, resp, sizeof(resp) - 1), PARSE_SUCCESS);
	sockaddr_storage from{};
	auto *sin = reinterpret_cast<sockaddr_in *>(&from);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sin->sin_port = htons(1900);
	ssdp_handle_ctrlpt_msg(&parser.msg, &from, 0);
	httpmsg_destroy(&parser.msg);
}

} // namespace

TEST(SsdpCtrlptTestSuite, no_search_result_after_unregister)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";
	ResetEvents();

	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(ctrlpt_callback, nullptr, &handle),
		UPNP_E_SUCCESS);
	int cookie = 0;
	// The M-SEARCH may not leave the host; only the search entry matters.
	(void)UpnpSearchAsync(handle, 5, "ssdp:all", &cookie);

	WorkerGate busy;
	ASSERT_TRUE(busy.Fill(&gRecvThreadPool, HIGH_PRIORITY));
	DeliverSearchResponse();
	ASSERT_EQ(UpnpUnRegisterClient(handle), UPNP_E_SUCCESS);
	g_ended = true;
	busy.Release();

	// Every job queued before these ones has finished once all of them
	// run at the same time.
	WorkerGate drain;
	ASSERT_TRUE(drain.Fill(&gRecvThreadPool, LOW_PRIORITY));
	drain.Release();

	EXPECT_EQ(g_late_results, 0);
	UpnpFinish();
}

TEST(SsdpCtrlptTestSuite, no_search_result_after_timeout)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";
	ResetEvents();

	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(ctrlpt_callback, nullptr, &handle),
		UPNP_E_SUCCESS);
	int cookie = 0;
	// Shortest search: it times out after MIN_SEARCH_TIME seconds.
	(void)UpnpSearchAsync(handle, 1, "ssdp:all", &cookie);

	WorkerGate busy;
	ASSERT_TRUE(busy.Fill(&gRecvThreadPool, HIGH_PRIORITY));
	DeliverSearchResponse();
	// The timeout runs on gSendThreadPool, so it is not held by the gate.
	auto deadline = std::chrono::steady_clock::now() +
			std::chrono::seconds(MIN_SEARCH_TIME + 10);
	while (!g_timed_out && std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	busy.Release();
	ASSERT_TRUE(g_timed_out);

	WorkerGate drain;
	ASSERT_TRUE(drain.Fill(&gRecvThreadPool, LOW_PRIORITY));
	drain.Release();

	EXPECT_EQ(g_late_results, 0);
	ASSERT_EQ(UpnpUnRegisterClient(handle), UPNP_E_SUCCESS);
	UpnpFinish();
}

namespace
{

// Holds the advertisement callback until the test opens the gate.
std::mutex g_adv_m;
std::condition_variable g_adv_cv;
bool g_adv_entered{false};
bool g_adv_open{false};

int adv_callback(Upnp_EventType type, void *, void *)
{
	if (type != UPNP_DISCOVERY_ADVERTISEMENT_ALIVE)
		return 0;
	std::unique_lock<std::mutex> lk(g_adv_m);
	g_adv_entered = true;
	g_adv_cv.notify_all();
	g_adv_cv.wait(lk, [] { return g_adv_open; });
	return 0;
}

// Parses an advertisement the way readFromSSDPSocket() does and hands it to
// the control point.
void DeliverAlive()
{
	static const char req[] = "NOTIFY * HTTP/1.1\r\n"
				  "HOST: 239.255.255.250:1900\r\n"
				  "CACHE-CONTROL: max-age=1800\r\n"
				  "LOCATION: http://127.0.0.1:1/desc.xml\r\n"
				  "NT: upnp:rootdevice\r\n"
				  "NTS: ssdp:alive\r\n"
				  "SERVER: test/1.0 UPnP/1.0\r\n"
				  "USN: uuid:test::upnp:rootdevice\r\n"
				  "\r\n";
	http_parser_t parser;
	parser_request_init(&parser);
	parse_status_t status = parser_append(&parser, req, sizeof(req) - 1);
	// A NOTIFY has no body: start_event_handler() accepts this failure.
	if (status == PARSE_SUCCESS ||
		(status == PARSE_FAILURE && parser.valid_ssdp_notify_hack)) {
		sockaddr_storage from{};
		auto *sin = reinterpret_cast<sockaddr_in *>(&from);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		sin->sin_port = htons(1900);
		ssdp_handle_ctrlpt_msg(&parser.msg, &from, 0);
	}
	httpmsg_destroy(&parser.msg);
}

} // namespace

TEST(SsdpCtrlptTestSuite, unregister_waits_for_advertisement_callback)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";

	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(adv_callback, nullptr, &handle),
		UPNP_E_SUCCESS);

	std::thread advertiser(DeliverAlive);
	bool entered;
	{
		std::unique_lock<std::mutex> lk(g_adv_m);
		entered = g_adv_cv.wait_for(lk, std::chrono::seconds(10), [] {
			return g_adv_entered;
		});
	}
	if (!entered) {
		advertiser.join();
		FAIL() << "the advertisement callback did not run";
	}

	std::atomic<bool> returned{false};
	std::thread unregister([&] {
		UpnpUnRegisterClient(handle);
		returned = true;
	});
	// UpnpUnRegisterClient() must still be waiting for the callback.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	bool returned_early = returned;
	{
		std::lock_guard<std::mutex> lk(g_adv_m);
		g_adv_open = true;
	}
	g_adv_cv.notify_all();
	advertiser.join();
	unregister.join();

	EXPECT_FALSE(returned_early);
	UpnpFinish();
}

namespace
{

// Holds the search timeout callback until the test opens the gate.
std::mutex g_to_m;
std::condition_variable g_to_cv;
bool g_to_entered{false};
bool g_to_open{false};

int timeout_callback(Upnp_EventType type, void *, void *)
{
	if (type != UPNP_DISCOVERY_SEARCH_TIMEOUT)
		return 0;
	std::unique_lock<std::mutex> lk(g_to_m);
	g_to_entered = true;
	g_to_cv.notify_all();
	g_to_cv.wait(lk, [] { return g_to_open; });
	return 0;
}

} // namespace

TEST(SsdpCtrlptTestSuite, unregister_waits_for_search_timeout_callback)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";

	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(timeout_callback, nullptr, &handle),
		UPNP_E_SUCCESS);
	int cookie = 0;
	// Shortest search: it times out after MIN_SEARCH_TIME seconds.
	(void)UpnpSearchAsync(handle, 1, "ssdp:all", &cookie);

	bool entered;
	{
		std::unique_lock<std::mutex> lk(g_to_m);
		entered = g_to_cv.wait_for(lk,
			std::chrono::seconds(MIN_SEARCH_TIME + 10),
			[] { return g_to_entered; });
	}
	if (!entered) {
		UpnpFinish();
		FAIL() << "the search timeout callback did not run";
	}

	std::atomic<bool> returned{false};
	std::thread unregister([&] {
		UpnpUnRegisterClient(handle);
		returned = true;
	});
	// UpnpUnRegisterClient() must still be waiting for the callback.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	bool returned_early = returned;
	{
		std::lock_guard<std::mutex> lk(g_to_m);
		g_to_open = true;
	}
	g_to_cv.notify_all();
	unregister.join();

	EXPECT_FALSE(returned_early);
	UpnpFinish();
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
