// UpnpUnRegisterRootDevice() promises that the SDK makes no more callbacks
// to the application once it returns (upnp.h). The device callbacks for a
// SOAP action, a QueryStateVariable request and a SUBSCRIBE request are made
// after the handle lock is released, without any tracking, so
// UpnpUnRegisterRootDevice() could return while one of them was still running
// and the application could free the cookie it was using.
//
// Each test registers a device, sends it a request over the network that
// holds the device callback, unregisters from another thread and checks that
// UpnpUnRegisterRootDevice() has not returned 500 ms later.

#include "upnp.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace
{

#define CONTROL_PATH "/control/unregister"
#define EVENT_PATH "/event/unregister"
#define SERVICE_TYPE "urn:schemas-upnp-org:service:Basic:1"

const char device_desc[] =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
	"<friendlyName>unregister</friendlyName>"
	"<manufacturer>Test</manufacturer>"
	"<modelName>Test</modelName>"
	"<UDN>uuid:unregist-0000-0000-0000-000000000000</UDN>"
	"<serviceList><service>"
	"<serviceType>" SERVICE_TYPE "</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:Basic</serviceId>"
	"<SCPDURL>/scpd.xml</SCPDURL>"
	"<controlURL>" CONTROL_PATH "</controlURL>"
	"<eventSubURL>" EVENT_PATH "</eventSubURL>"
	"</service></serviceList>"
	"</device>"
	"</root>";

// Holds the device callback of type g_held until the test opens the gate.
std::mutex g_m;
std::condition_variable g_cv;
Upnp_EventType g_held;
bool g_entered{false};
bool g_open{false};

int device_callback(Upnp_EventType type, void *, void *)
{
	if (type != g_held)
		return 0;
	std::unique_lock<std::mutex> lk(g_m);
	g_entered = true;
	g_cv.notify_all();
	g_cv.wait(lk, [] { return g_open; });
	return 0;
}

std::string HostPort()
{
	return std::string(UpnpGetServerIpAddress()) + ":" +
	       std::to_string(UpnpGetServerPort());
}

std::string SoapRequest(const std::string &soap_action, const std::string &body)
{
	std::string envelope =
		"<?xml version=\"1.0\"?>"
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle="
		"\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>" +
		body +
		"</s:Body>"
		"</s:Envelope>";
	return "POST " CONTROL_PATH " HTTP/1.1\r\n"
	       "HOST: " +
	       HostPort() +
	       "\r\n"
	       "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
	       "SOAPACTION: \"" +
	       soap_action +
	       "\"\r\n"
	       "CONTENT-LENGTH: " +
	       std::to_string(envelope.size()) + "\r\n\r\n" + envelope;
}

// Sends a request to the device and waits for the reply.
void SendRequest(std::string req)
{
	sockaddr_in addr{};
	timeval tv{};
	char reply[1024];

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return;
	tv.tv_sec = 10;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
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

// Registers the device, sends the request made by make_req, and once the
// callback of type held is running unregisters the device from another thread
// and checks that UpnpUnRegisterRootDevice() waits for the callback.
void CheckUnregisterWaits(
	Upnp_EventType held, const std::function<std::string()> &make_req)
{
	g_held = held;
	g_entered = false;
	g_open = false;

	if (UpnpInit2(nullptr, 0) != UPNP_E_SUCCESS)
		GTEST_SKIP() << "UpnpInit2 failed (no network?)";

	UpnpDevice_Handle handle = -1;
	ASSERT_EQ(UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
			  device_desc,
			  sizeof(device_desc) - 1,
			  1,
			  device_callback,
			  nullptr,
			  &handle),
		UPNP_E_SUCCESS);

	std::thread client(SendRequest, make_req());
	bool entered;
	{
		std::unique_lock<std::mutex> lk(g_m);
		entered = g_cv.wait_for(
			lk, std::chrono::seconds(10), [] { return g_entered; });
	}
	if (!entered) {
		client.join();
		UpnpFinish();
		FAIL() << "the device callback did not run";
	}

	std::atomic<bool> returned{false};
	std::thread unregister([&] {
		UpnpUnRegisterRootDevice(handle);
		returned = true;
	});
	// UpnpUnRegisterRootDevice() must still be waiting for the callback.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	bool returned_early = returned;
	{
		std::lock_guard<std::mutex> lk(g_m);
		g_open = true;
	}
	g_cv.notify_all();
	client.join();
	unregister.join();

	EXPECT_FALSE(returned_early);
	UpnpFinish();
}

} // namespace

TEST(DeviceUnregisterTestSuite, unregister_waits_for_action_callback)
{
	CheckUnregisterWaits(UPNP_CONTROL_ACTION_REQUEST, [] {
		return SoapRequest(SERVICE_TYPE "#Hold",
			"<u:Hold xmlns:u=\"" SERVICE_TYPE "\"></u:Hold>");
	});
}

TEST(DeviceUnregisterTestSuite, unregister_waits_for_query_variable_callback)
{
	CheckUnregisterWaits(UPNP_CONTROL_GET_VAR_REQUEST, [] {
		return SoapRequest(
			"urn:schemas-upnp-org:control-1-0#QueryStateVariable",
			"<u:QueryStateVariable "
			"xmlns:u=\"urn:schemas-upnp-org:control-1-0\">"
			"<u:varName>Status</u:varName>"
			"</u:QueryStateVariable>");
	});
}

TEST(DeviceUnregisterTestSuite, unregister_waits_for_subscription_callback)
{
	// The delivery URL must be a literal address in the device's subnet.
	CheckUnregisterWaits(UPNP_EVENT_SUBSCRIPTION_REQUEST, [] {
		return std::string("SUBSCRIBE " EVENT_PATH " HTTP/1.1\r\n"
				   "HOST: ") +
		       HostPort() +
		       "\r\n"
		       "CALLBACK: <http://" +
		       UpnpGetServerIpAddress() +
		       ":9/>\r\n"
		       "NT: upnp:event\r\n"
		       "TIMEOUT: Second-1800\r\n"
		       "\r\n";
	});
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
