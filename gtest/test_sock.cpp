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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
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
//
// The send() is blocking, so if the reader ever stopped before EOF the test
// would hang until ctest kills it (seen once on macOS). SO_SNDTIMEO turns
// that into a failed send(), and the reader retries on EINTR and reports
// any other error.
//
// It still hung once more on macOS, with nothing printed, so the hang is not
// in a blocked send(). A watchdog now reports where the test stopped before
// ctest kills it: if the test has not finished after 20 s (it normally takes
// about 2.5 s), it prints the stage, what sock_write() returned, what the
// reader received and what is still unread, and aborts.
TEST_F(Issue395TestSuite, ghsa_323w_budget_never_goes_negative)
{
	SOCKINFO info{};
	info.socket = sv[0];
	int timeout = 1;
	std::vector<char> big(4 * 1024 * 1024, 'x');
	struct timeval sndtimeo = {5, 0};
	int reader_errno = 0;

	ASSERT_EQ(setsockopt(sv[0],
			  SOL_SOCKET,
			  SO_SNDTIMEO,
			  &sndtimeo,
			  sizeof(sndtimeo)),
		0);
	/* 0 writing, 1 sock_write() returned, 2 shut down, 3 reader joined */
	std::atomic<int> stage{0};
	std::atomic<int> writer_rc{0};
	std::atomic<long> received{0};
	std::atomic<bool> reader_done{false};
	std::mutex finished_mutex;
	std::condition_variable finished_cv;
	bool finished = false;
	std::thread watchdog([&] {
		std::unique_lock<std::mutex> lock(finished_mutex);
		if (finished_cv.wait_for(lock,
			    std::chrono::seconds(20),
			    [&finished] { return finished; }))
			return;
		int unread = -1;
		ioctl(sv[1], FIONREAD, &unread);
		fprintf(stderr,
			"ghsa_323w watchdog: not finished after 20 s: stage "
			"%d, "
			"sock_write() returned %d, reader got %ld of %zu bytes "
			"and is %s, %d bytes unread\n",
			stage.load(),
			writer_rc.load(),
			received.load(),
			big.size(),
			reader_done.load() ? "done" : "still reading",
			unread);
		fflush(stderr);
		abort();
	});
	std::thread reader([this, &reader_errno, &received, &reader_done] {
		char buf[65536];
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		for (;;) {
			ssize_t n = recv(sv[1], buf, sizeof(buf), 0);
			if (n > 0) {
				received += n;
				continue;
			}
			if (n < 0 && errno == EINTR)
				continue;
			if (n < 0)
				reader_errno = errno;
			break;
		}
		reader_done = true;
	});
	int rc = sock_write(&info, big.data(), big.size(), &timeout);
	writer_rc = rc;
	stage = 1;
	shutdown(sv[0], SHUT_WR);
	stage = 2;
	reader.join();
	stage = 3;
	{
		std::lock_guard<std::mutex> lock(finished_mutex);
		finished = true;
	}
	finished_cv.notify_one();
	watchdog.join();

	EXPECT_EQ(reader_errno, 0) << strerror(reader_errno);
	ASSERT_EQ(rc, (int)big.size());
	EXPECT_GE(timeout, 0);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
