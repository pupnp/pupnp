// regression: issue #395
// sock_read_write() passed MSG_DONTROUTE to send() unconditionally.
// MSG_DONTROUTE is a no-op for TCP on Linux but causes send() to fail
// on Fuchsia (EPFNOSUPPORT), making UpnpRegisterRootDevice4 always fail.
//
// Fix: remove MSG_DONTROUTE from the send() call in sock_read_write().
// SSDP UDP sends go through sendto() directly and are unaffected.

#include "gtest/gtest.h"
#include <stdio.h>

extern "C" {
#include "sock.h"
}

#include <chrono>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

class Issue395TestSuite : public ::testing::Test
{
protected:
	int sv[2]{-1, -1};

	void SetUp() override
	{
		ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
	}

	void TearDown() override
	{
		if (sv[0] >= 0)
			close(sv[0]);
		if (sv[1] >= 0)
			close(sv[1]);
	}
};

// regression: issue #395
// sock_write must succeed on a connected TCP-like socket.
// On Fuchsia this failed because MSG_DONTROUTE is unsupported.
TEST_F(Issue395TestSuite, sock_write_succeeds_over_socketpair)
{
	SOCKINFO info{};
	info.socket = sv[0];
	int timeout = 5;
	const char msg[] = "hello";

	int rc = sock_write(&info, msg, sizeof(msg) - 1, &timeout);
	EXPECT_EQ(rc, (int)(sizeof(msg) - 1));
}

TEST_F(Issue395TestSuite, sock_write_data_received_intact)
{
	SOCKINFO info{};
	info.socket = sv[0];
	int timeout = 5;
	const char msg[] = "hello";

	int rc = sock_write(&info, msg, sizeof(msg) - 1, &timeout);
	ASSERT_EQ(rc, (int)(sizeof(msg) - 1));

	char buf[16]{};
	ssize_t n = recv(sv[1], buf, sizeof(buf), 0);
	ASSERT_EQ(n, (ssize_t)(sizeof(msg) - 1));
	EXPECT_STREQ(buf, "hello");
}

// regression: GHSA-323w-xv8r-6g77
// sock_read_write() subtracted the elapsed whole seconds from *timeoutSecs
// without clamping, so a positive budget could become negative, which the
// next call treats as "no timeout" and poll() waits forever.  A blocking
// send() to a slow reader takes longer than the budget and overcharges it.
TEST_F(Issue395TestSuite, ghsa_323w_budget_never_goes_negative)
{
	SOCKINFO info{};
	info.socket = sv[0];
	int timeout = 1;
	std::vector<char> big(4 * 1024 * 1024, 'x');

	std::thread reader([this] {
		char buf[65536];
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		while (recv(sv[1], buf, sizeof(buf), 0) > 0) {
		}
	});
	int rc = sock_write(&info, big.data(), big.size(), &timeout);
	shutdown(sv[0], SHUT_WR);
	reader.join();

	ASSERT_EQ(rc, (int)big.size());
	EXPECT_GE(timeout, 0);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
