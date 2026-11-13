// UpnpUnRegisterClient() promises that no callback follows its return, and
// waits for the discovery callbacks that are running (GHSA-mrw2-788c-f6w7,
// PR #674). gena_process_notification_event() also releases the handle lock
// before it calls the control point back with UPNP_EVENT_RECEIVED, but that
// callback was not tracked, so UpnpUnRegisterClient() could return while it
// was still running and the application could free the cookie it was using.
//
// A fake publisher grants a subscription, the test sends a NOTIFY to the
// control point and holds the event callback, unregisters from another thread
// and checks that UpnpUnRegisterClient() has not returned 500 ms later.

#include "upnp.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{

#define TEST_SID "uuid:gena-ctrlpt-test"

// Holds the event callback until the test opens the gate.
std::mutex g_event_m;
std::condition_variable g_event_cv;
bool g_event_entered{false};
bool g_event_open{false};

int event_callback(Upnp_EventType type, void *, void *)
{
	if (type != UPNP_EVENT_RECEIVED)
		return 0;
	std::unique_lock<std::mutex> lk(g_event_m);
	g_event_entered = true;
	g_event_cv.notify_all();
	g_event_cv.wait(lk, [] { return g_event_open; });
	return 0;
}

// Answers every request (SUBSCRIBE, UNSUBSCRIBE) with 200 and TEST_SID.
class FakePublisher
{
public:
	~FakePublisher() { Stop(); }

	bool Start()
	{
		sockaddr_in addr{};
		socklen_t len = sizeof(addr);

		fd_ = socket(AF_INET, SOCK_STREAM, 0);
		if (fd_ < 0)
			return false;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (bind(fd_,
			    reinterpret_cast<sockaddr *>(&addr),
			    sizeof(addr)) != 0 ||
			listen(fd_, 4) != 0 ||
			getsockname(fd_,
				reinterpret_cast<sockaddr *>(&addr),
				&len) != 0)
			return false;
		url_ = "http://127.0.0.1:" +
		       std::to_string(ntohs(addr.sin_port)) + "/event";
		thread_ = std::thread(&FakePublisher::Run, this);
		return true;
	}

	void Stop()
	{
		stop_ = true;
		if (thread_.joinable())
			thread_.join();
		if (fd_ >= 0)
			close(fd_);
		fd_ = -1;
	}

	const char *Url() const { return url_.c_str(); }

private:
	void Run()
	{
		static const char resp[] = "HTTP/1.1 200 OK\r\n"
					   "SID: " TEST_SID "\r\n"
					   "TIMEOUT: Second-1800\r\n"
					   "Content-Length: 0\r\n"
					   "\r\n";
		while (!stop_) {
			pollfd pfd{fd_, POLLIN, 0};
			if (poll(&pfd, 1, 100) <= 0)
				continue;
			int c = accept(fd_, nullptr, nullptr);
			if (c < 0)
				continue;
			// Read up to the end of the request headers.
			std::string req;
			char buf[1024];
			while (req.find("\r\n\r\n") == std::string::npos) {
				ssize_t n = recv(c, buf, sizeof(buf), 0);
				if (n <= 0)
					break;
				req.append(buf, static_cast<size_t>(n));
			}
			send(c, resp, sizeof(resp) - 1, 0);
			close(c);
		}
	}

	int fd_{-1};
	std::string url_;
	std::atomic<bool> stop_{false};
	std::thread thread_;
};

// Sends a NOTIFY for TEST_SID to the control point and waits for the reply,
// which comes once the event callback has returned.
void SendNotify()
{
	static const char body[] =
		"<?xml version=\"1.0\"?>\r\n"
		"<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
		"<e:property><Status>1</Status></e:property>"
		"</e:propertyset>\r\n";
	std::string req = std::string("NOTIFY / HTTP/1.1\r\n"
				      "HOST: ") +
			  UpnpGetServerIpAddress() + ":" +
			  std::to_string(UpnpGetServerPort()) +
			  "\r\n"
			  "CONTENT-TYPE: text/xml\r\n"
			  "CONTENT-LENGTH: " +
			  std::to_string(sizeof(body) - 1) +
			  "\r\n"
			  "NT: upnp:event\r\n"
			  "NTS: upnp:propchange\r\n"
			  "SEQ: 1\r\n"
			  "SID: " TEST_SID "\r\n"
			  "\r\n" +
			  body;
	sockaddr_in addr{};
	char reply[256];

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(UpnpGetServerPort());
	if (inet_pton(AF_INET, UpnpGetServerIpAddress(), &addr.sin_addr) == 1 &&
		connect(fd,
			reinterpret_cast<sockaddr *>(&addr),
			sizeof(addr)) == 0 &&
		send(fd, req.data(), req.size(), 0) ==
			static_cast<ssize_t>(req.size()))
		(void)recv(fd, reply, sizeof(reply), 0);
	close(fd);
}

} // namespace

TEST(GenaCtrlptTestSuite, unregister_waits_for_event_callback)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";

	FakePublisher publisher;
	ASSERT_TRUE(publisher.Start());
	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(event_callback, nullptr, &handle),
		UPNP_E_SUCCESS);
	int timeout = 1800;
	Upnp_SID sid;
	ASSERT_EQ(UpnpSubscribe(handle, publisher.Url(), &timeout, sid),
		UPNP_E_SUCCESS);

	std::thread notifier(SendNotify);
	bool entered;
	{
		std::unique_lock<std::mutex> lk(g_event_m);
		entered = g_event_cv.wait_for(lk, std::chrono::seconds(10), [] {
			return g_event_entered;
		});
	}
	if (!entered) {
		notifier.join();
		UpnpFinish();
		FAIL() << "the event callback did not run";
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
		std::lock_guard<std::mutex> lk(g_event_m);
		g_event_open = true;
	}
	g_event_cv.notify_all();
	notifier.join();
	unregister.join();

	EXPECT_FALSE(returned_early);
	UpnpFinish();
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
