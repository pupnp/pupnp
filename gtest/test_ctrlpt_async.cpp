// UpnpUnRegisterClient() promises that no callback follows its return. The
// asynchronous control point calls (UpnpSendActionAsync() and the others run
// by UpnpThreadDistribution()) called their completion callback without
// checking the handle and without marking the callback as running, so
// UpnpUnRegisterClient() did not wait for a running completion callback, and
// an operation still in progress called back after the client was gone.
//
// A fake control URL answers every request, after an optional delay.

#include "upnp.h"
#include "upnptools.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
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

// Answers every request with an empty 200 after delay_ms.
class FakeServer
{
public:
	explicit FakeServer(int delay_ms) : delay_ms_(delay_ms) {}
	~FakeServer() { Stop(); }

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
		       std::to_string(ntohs(addr.sin_port)) + "/control";
		thread_ = std::thread(&FakeServer::Run, this);
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
			std::this_thread::sleep_for(
				std::chrono::milliseconds(delay_ms_));
			send(c, resp, sizeof(resp) - 1, 0);
			close(c);
		}
	}

	int delay_ms_;
	int fd_{-1};
	std::string url_;
	std::atomic<bool> stop_{false};
	std::thread thread_;
};

int client_callback(Upnp_EventType, void *, void *) { return 0; }

// Holds UPNP_CONTROL_ACTION_COMPLETE until the test opens the gate.
std::mutex g_m;
std::condition_variable g_cv;
int g_calls{0};
bool g_hold{false};
bool g_open{false};

int action_callback(Upnp_EventType type, void *, void *)
{
	if (type != UPNP_CONTROL_ACTION_COMPLETE)
		return 0;
	std::unique_lock<std::mutex> lk(g_m);
	++g_calls;
	g_cv.notify_all();
	if (g_hold)
		g_cv.wait(lk, [] { return g_open; });
	return 0;
}

void ResetCallbackState(bool hold)
{
	std::lock_guard<std::mutex> lk(g_m);
	g_calls = 0;
	g_hold = hold;
	g_open = false;
}

int SendAction(UpnpClient_Handle handle, const char *url)
{
	IXML_Document *act = UpnpMakeAction(
		"GetStatus", "urn:schemas-upnp-org:service:Test:1", 0, nullptr);
	if (act == nullptr)
		return UPNP_E_OUTOF_MEMORY;
	int rc = UpnpSendActionAsync(handle,
		url,
		"urn:schemas-upnp-org:service:Test:1",
		nullptr,
		act,
		action_callback,
		nullptr);
	ixmlDocument_free(act);
	return rc;
}

} // namespace

TEST(CtrlptAsyncTestSuite, unregister_waits_for_action_complete_callback)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";

	FakeServer server(0);
	ASSERT_TRUE(server.Start());
	ResetCallbackState(true);
	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(client_callback, nullptr, &handle),
		UPNP_E_SUCCESS);
	ASSERT_EQ(SendAction(handle, server.Url()), UPNP_E_SUCCESS);

	bool entered;
	{
		std::unique_lock<std::mutex> lk(g_m);
		entered = g_cv.wait_for(lk, std::chrono::seconds(10), [] {
			return g_calls > 0;
		});
	}
	if (!entered) {
		UpnpFinish();
		FAIL() << "the action complete callback did not run";
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
		std::lock_guard<std::mutex> lk(g_m);
		g_open = true;
	}
	g_cv.notify_all();
	unregister.join();

	EXPECT_FALSE(returned_early);
	UpnpFinish();
}

TEST(CtrlptAsyncTestSuite, no_action_complete_callback_after_unregister)
{
	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";

	FakeServer server(1000);
	ASSERT_TRUE(server.Start());
	ResetCallbackState(false);
	UpnpClient_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterClient(client_callback, nullptr, &handle),
		UPNP_E_SUCCESS);
	ASSERT_EQ(SendAction(handle, server.Url()), UPNP_E_SUCCESS);
	ASSERT_EQ(UpnpUnRegisterClient(handle), UPNP_E_SUCCESS);

	// The server answers after 1 s; give the job time to finish.
	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	int calls;
	{
		std::lock_guard<std::mutex> lk(g_m);
		calls = g_calls;
	}
	EXPECT_EQ(calls, 0);
	UpnpFinish();
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
