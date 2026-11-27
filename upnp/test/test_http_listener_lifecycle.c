/* HTTP listeners are optional independently of SSDP. The XML server below
 * belongs to the application, not pupnp. No external network is needed. */
#include "upnp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <ifaddrs.h>
	#include <net/if.h>
	#include <poll.h>
	#include <signal.h>
	#include <stdatomic.h>
	#include <sys/resource.h>
	#include <sys/socket.h>
	#include <sys/wait.h>
	#include <unistd.h>

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

	#define CHECK(expr) \
		do { \
			if (!(expr)) { \
				fprintf(stderr, \
					"FAIL line %d: %s\n", \
					__LINE__, \
					#expr); \
				goto cleanup; \
			} \
		} while (0)

static const char xml[] =
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device><deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>External BasicDevice</friendlyName>"
	"<manufacturer>Test</manufacturer><modelName>Test</modelName>"
	"<UDN>uuid:listener-lifecycle-test</UDN></device></root>";

static int event(Upnp_EventType type, void *data, void *cookie)
{
	(void)type;
	(void)data;
	(void)cookie;
	return 0;
}

	#ifdef UPNP_HAVE_WEBSERVER
static _Atomic int callback_result = 1;

static int disable_from_callback(const char *host, void *cookie)
{
	(void)host;
	(void)cookie;
	atomic_store(&callback_result, UpnpEnableWebserver(0));
	return UPNP_E_BAD_HTTPMSG;
}
	#endif

/* Loopback is lo on Linux and lo0 on macOS/BSD. */
static int find_loopback(char *name, size_t name_size)
{
	struct ifaddrs *interfaces, *iface;
	int found = 0;
	if (getifaddrs(&interfaces) != 0)
		return 0;
	for (iface = interfaces; iface; iface = iface->ifa_next) {
		if (!iface->ifa_addr || iface->ifa_addr->sa_family != AF_INET ||
			!(iface->ifa_flags & IFF_LOOPBACK) ||
			!(iface->ifa_flags & IFF_UP))
			continue;
		snprintf(name, name_size, "%s", iface->ifa_name);
		found = 1;
		break;
	}
	freeifaddrs(interfaces);
	return found;
}

static int tcp_socket(unsigned short port, int listening)
{
	struct sockaddr_in addr;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	if (listening) {
		int one = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	if ((listening ? bind(fd, (struct sockaddr *)&addr, sizeof(addr))
		       : connect(fd, (struct sockaddr *)&addr, sizeof(addr))) <
			0 ||
		(listening && listen(fd, 8) < 0)) {
		close(fd);
		return -1;
	}
	return fd;
}

static unsigned short socket_port(int fd)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr *)&addr, &len))
		return 0;
	return ntohs(addr.sin_port);
}

static void serve_xml(int listener, pid_t parent)
{
	for (;;) {
		char request[4096], response[8192];
		struct pollfd pfd = {listener, POLLIN, 0};
		int fd;
		/* Do not leave a fixture behind if ASan aborts the parent. */
		if (getppid() != parent)
			_exit(0);
		if (poll(&pfd, 1, 100) <= 0)
			continue;
		fd = accept(listener, NULL, NULL);
		int len;
		if (fd < 0)
			_exit(1);
		if (recv(fd, request, sizeof(request), 0) > 0) {
			len = snprintf(response,
				sizeof(response),
				"HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\n"
				"Content-Length: %zu\r\nConnection: "
				"close\r\n\r\n%s",
				strlen(xml),
				xml);
			if (send(fd, response, (size_t)len, MSG_NOSIGNAL) !=
				len)
				_exit(1);
		}
		close(fd);
	}
}

static int ssdp_observer(void)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0), one = 1;
	struct sockaddr_in addr;
	struct ip_mreq group;
	if (fd < 0)
		return -1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
		close(fd);
		return -1;
	}
	#if (defined(BSD) && !defined(__GNU__)) || defined(__APPLE__)
	/* Match the SDK's SSDP socket: BSD/macOS require SO_REUSEPORT on
	 * both sockets sharing the multicast port. Do not set it on TCP
	 * probes, which must still detect an occupied listener port. */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one))) {
		close(fd);
		return -1;
	}
	#endif
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1900);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&group, 0, sizeof(group));
	inet_pton(AF_INET, "239.255.255.250", &group.imr_multiaddr);
	group.imr_interface.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) ||
		setsockopt(fd,
			IPPROTO_IP,
			IP_ADD_MEMBERSHIP,
			&group,
			sizeof(group))) {
		close(fd);
		return -1;
	}
	return fd;
}

static int receive_location(int fd, const char *url, const char *kind)
{
	struct pollfd pfd = {fd, POLLIN, 0};
	int attempt;
	for (attempt = 0; attempt < 20; ++attempt) {
		char packet[8192];
		ssize_t n;
		if (poll(&pfd, 1, 250) <= 0)
			continue;
		n = recv(fd, packet, sizeof(packet) - 1, 0);
		if (n <= 0)
			continue;
		packet[n] = 0;
		if (strstr(packet, url) && strstr(packet, kind))
			return 1;
	}
	return 0;
}

static int search_device(const char *url)
{
	static const char search[] =
		"M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\n"
		"MAN: \"ssdp:discover\"\r\nMX: 1\r\n"
		"ST: urn:schemas-upnp-org:device:Basic:1\r\n\r\n";
	int fd = socket(AF_INET, SOCK_DGRAM, 0), ok;
	struct sockaddr_in dst, src;
	struct in_addr iface;
	if (fd < 0)
		return 0;
	memset(&src, 0, sizeof(src));
	src.sin_family = AF_INET;
	src.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr *)&src, sizeof(src))) {
		close(fd);
		return 0;
	}
	iface.s_addr = htonl(INADDR_LOOPBACK);
	setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface));
	memset(&dst, 0, sizeof(dst));
	dst.sin_family = AF_INET;
	dst.sin_port = htons(1900);
	inet_pton(AF_INET, "239.255.255.250", &dst.sin_addr);
	sendto(fd,
		search,
		sizeof(search) - 1,
		0,
		(struct sockaddr *)&dst,
		sizeof(dst));
	ok = receive_location(fd, url, "200 OK");
	close(fd);
	return ok;
}

	#ifdef UPNP_HAVE_WEBSERVER
/* Force listener socket creation to fail, then prove rollback/recovery. */
static int transition_under_fd_exhaustion(int enable)
{
	struct rlimit original, limited;
	int fds[128], count = 0, result;
	if (getrlimit(RLIMIT_NOFILE, &original))
		return 0;
	limited = original;
	if (limited.rlim_cur > 128)
		limited.rlim_cur = 128;
	if (setrlimit(RLIMIT_NOFILE, &limited))
		return 0;
	while (count < 128) {
		int fd = open("/dev/null", O_RDONLY);
		if (fd < 0)
			break;
		fds[count++] = fd;
	}
	result = UpnpEnableWebserver(enable);
	while (count)
		close(fds[--count]);
	if (setrlimit(RLIMIT_NOFILE, &original))
		return 0;
	return result == (enable ? UPNP_E_OUTOF_SOCKET : UPNP_E_SUCCESS) &&
	       !UpnpIsWebserverEnabled() && !UpnpGetServerPort();
}
	#endif

int main(void)
{
	int result = EXIT_FAILURE, initialized = 0, registered = 0;
	int external = -1, observer = -1, idle = -1, probe = -1;
	pid_t child = -1, parent = getpid();
	unsigned short reserved;
	char url[256];
	char name[IF_NAMESIZE + 1];
	UpnpDevice_Handle device;
	#ifdef UPNP_HAVE_CLIENT
	UpnpClient_Handle client;
		#ifdef UPNP_HAVE_GENA
	Upnp_SID sid;
	int timeout = 60;
		#endif
	#endif
	#ifdef UPNP_HAVE_WEBSERVER
	unsigned short port;
	int i;
	struct pollfd pfd;
	char byte;
	#endif

	if (!find_loopback(name, sizeof(name))) {
		puts("SKIP: no IPv4 loopback interface is up");
		return EXIT_SUCCESS;
	}
	printf("Listener lifecycle test on loopback interface %s\n", name);

	signal(SIGPIPE, SIG_IGN);
	alarm(60);
	CHECK(UpnpEnableWebserver(0) == UPNP_E_FINISH);
	CHECK(UpnpInit3(name, 0, ~UPNP_INIT_NO_WEB_SERVER) ==
		UPNP_E_INVALID_PARAM);
	CHECK(!UpnpIsWebserverEnabled());
	external = tcp_socket(0, 1);
	CHECK(external >= 0);
	snprintf(url,
		sizeof(url),
		"http://127.0.0.1:%u/device.xml",
		socket_port(external));
	child = fork();
	CHECK(child >= 0);
	if (!child)
		serve_xml(external, parent);
	close(external);
	external = -1;
	probe = tcp_socket(0, 1);
	CHECK(probe >= 0);
	reserved = socket_port(probe);
	close(probe);
	probe = -1;
	observer = ssdp_observer();
	CHECK(observer >= 0);
	CHECK(UpnpInit3(name, reserved, UPNP_INIT_NO_WEB_SERVER) ==
		UPNP_E_SUCCESS);
	initialized = 1;
	CHECK(UpnpInit2(name, 0) == UPNP_E_INIT);
	CHECK(!UpnpIsWebserverEnabled());
	CHECK(!UpnpGetServerPort() && !UpnpGetServerPort6() &&
		!UpnpGetServerUlaGuaPort6());
	probe = tcp_socket(
		reserved, 1); /* proves the requested port was not bound */
	CHECK(probe >= 0);
	close(probe);
	probe = -1;
	CHECK(UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
		      xml,
		      strlen(xml),
		      1,
		      event,
		      NULL,
		      &device) == UPNP_E_NO_WEB_SERVER);
	CHECK(UpnpRegisterRootDevice3(url, event, NULL, &device, AF_INET) ==
		UPNP_E_SUCCESS);
	registered = 1;
	CHECK(UpnpSendAdvertisement(device, 1800) == UPNP_E_SUCCESS);
	CHECK(receive_location(observer, url, "ssdp:alive"));
	CHECK(search_device(url));
	CHECK(!UpnpGetServerPort());
	#ifdef UPNP_HAVE_CLIENT
	CHECK(UpnpRegisterClient(event, NULL, &client) == UPNP_E_SUCCESS);
		#ifdef UPNP_HAVE_GENA
	CHECK(UpnpSubscribe(client, url, &timeout, sid) ==
		UPNP_E_NO_WEB_SERVER);
	CHECK(UpnpSubscribeAsync(client, url, 60, event, NULL) ==
		UPNP_E_NO_WEB_SERVER);
		#endif
	CHECK(UpnpUnRegisterClient(client) == UPNP_E_SUCCESS);
	#endif
	#ifdef UPNP_HAVE_WEBSERVER
	CHECK(transition_under_fd_exhaustion(1));
	for (i = 0; i < 4; ++i) {
		if (i == 1) {
			probe = tcp_socket(reserved, 1);
			CHECK(probe >= 0);
		}
		CHECK(UpnpEnableWebserver(1) == UPNP_E_SUCCESS);
		CHECK(UpnpIsWebserverEnabled());
		port = UpnpGetServerPort();
		CHECK(port != 0);
		CHECK(i == 1 ? port != reserved : port == reserved);
		if (probe >= 0) {
			close(probe);
			probe = -1;
		}
		CHECK(UpnpEnableWebserver(1) == UPNP_E_SUCCESS);
		CHECK(UpnpGetServerPort() == port);
		idle = tcp_socket(port, 0);
		CHECK(idle >= 0);
		usleep(30000); /* let the listener accept the idle connection */
		if (i == 0)
			CHECK(transition_under_fd_exhaustion(0));
		else
			CHECK(UpnpEnableWebserver(0) == UPNP_E_SUCCESS);
		CHECK(UpnpEnableWebserver(0) == UPNP_E_SUCCESS);
		CHECK(!UpnpIsWebserverEnabled() && !UpnpGetServerPort());
		CHECK(!UpnpGetServerPort6() && !UpnpGetServerUlaGuaPort6());
		pfd.fd = idle;
		pfd.events = POLLIN;
		pfd.revents = 0;
		CHECK(poll(&pfd, 1, 1000) > 0);
		CHECK(recv(idle, &byte, 1, 0) <= 0);
		close(idle);
		idle = -1;
		probe = tcp_socket(port, 0);
		CHECK(probe < 0);
		CHECK(search_device(url));
	}
	CHECK(UpnpEnableWebserver(1) == UPNP_E_SUCCESS);
	UpnpSetHostValidateCallback(disable_from_callback, NULL);
	idle = tcp_socket(UpnpGetServerPort(), 0);
	CHECK(idle >= 0);
	{
		static const char request[] =
			"GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
		CHECK(send(idle, request, sizeof(request) - 1, MSG_NOSIGNAL) >
			0);
	}
	for (i = 0; i < 100 && atomic_load(&callback_result) == 1; ++i)
		usleep(10000);
	CHECK(atomic_load(&callback_result) == UPNP_E_SUCCESS);
	CHECK(!UpnpIsWebserverEnabled());
	close(idle);
	idle = -1;
	UpnpSetHostValidateCallback(NULL, NULL);
	#else
	CHECK(UpnpEnableWebserver(1) == UPNP_E_NO_WEB_SERVER);
	#endif
	CHECK(UpnpUnRegisterRootDevice(device) == UPNP_E_SUCCESS);
	registered = 0;
	CHECK(UpnpFinish() == UPNP_E_SUCCESS);
	initialized = 0;
	CHECK(!UpnpGetServerPort());
	CHECK(UpnpInit3("no-such-pupnp-interface", 0, 0) ==
		UPNP_E_INVALID_INTERFACE);
	CHECK(UpnpInit2(name, 0) == UPNP_E_SUCCESS);
	initialized = 1;
	#ifdef UPNP_HAVE_WEBSERVER
	CHECK(UpnpGetServerPort() != 0 && UpnpIsWebserverEnabled());
	CHECK(UpnpEnableWebserver(0) == UPNP_E_SUCCESS);
	CHECK(UpnpRegisterRootDevice3(url, event, NULL, &device, AF_INET) ==
		UPNP_E_SUCCESS);
	registered = 1;
	CHECK(UpnpSendAdvertisement(device, 1800) == UPNP_E_SUCCESS);
	CHECK(search_device(url));
	CHECK(!UpnpGetServerPort());
	CHECK(UpnpUnRegisterRootDevice(device) == UPNP_E_SUCCESS);
	registered = 0;
	#endif
	CHECK(UpnpFinish() == UPNP_E_SUCCESS);
	initialized = 0;
	CHECK(UpnpInit3(name, 0, UPNP_INIT_DEFAULT) == UPNP_E_SUCCESS);
	initialized = 1;
	#ifdef UPNP_HAVE_WEBSERVER
	CHECK(UpnpGetServerPort() != 0 && UpnpIsWebserverEnabled());
	#endif
	result = EXIT_SUCCESS;
cleanup:
	if (idle >= 0)
		close(idle);
	if (probe >= 0)
		close(probe);
	if (registered)
		UpnpUnRegisterRootDevice(device);
	if (initialized)
		UpnpFinish();
	if (external >= 0)
		close(external);
	if (observer >= 0)
		close(observer);
	if (child > 0) {
		kill(child, SIGTERM);
		waitpid(child, NULL, 0);
	}
	return result;
}
#else
int main(void)
{
	puts("SKIP: POSIX loopback fixture");
	return 0;
}
#endif
