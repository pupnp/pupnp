// regression: GHSA-8rmx-x2cq-xh33
//
// The HTTPS client must authenticate the server: the certificate chain is
// verified by default and the certificate must match the host of the URL.
// HTTP paths that have no TLS must not send an https:// request in clear.

#include "upnp.h"

#include "gtest/gtest.h"

#ifdef UPNP_ENABLE_OPEN_SSL

	#include <openssl/x509v3.h>

	#include <arpa/inet.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <poll.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>

	#include <atomic>
	#include <cstring>
	#include <string>
	#include <thread>

namespace
{

EVP_PKEY *make_key()
{
	EVP_PKEY *pkey = nullptr;
	EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
	if (kctx && EVP_PKEY_keygen_init(kctx) == 1 &&
		EVP_PKEY_CTX_set_ec_paramgen_curve_nid(
			kctx, NID_X9_62_prime256v1) == 1) {
		EVP_PKEY_keygen(kctx, &pkey);
	}
	EVP_PKEY_CTX_free(kctx);
	return pkey;
}

/* Self-signed certificate with the given subjectAltName. */
X509 *make_cert(EVP_PKEY *pkey, const char *san)
{
	X509 *x = X509_new();
	X509_set_version(x, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
	X509_gmtime_adj(X509_getm_notBefore(x), -60);
	X509_gmtime_adj(X509_getm_notAfter(x), 3600);
	X509_set_pubkey(x, pkey);
	X509_NAME *name = X509_get_subject_name(x);
	X509_NAME_add_entry_by_txt(name,
		"CN",
		MBSTRING_ASC,
		(const unsigned char *)"pupnp-test",
		-1,
		-1,
		0);
	X509_set_issuer_name(x, name);
	X509V3_CTX v3;
	X509V3_set_ctx_nodb(&v3);
	X509V3_set_ctx(&v3, x, x, nullptr, nullptr, 0);
	X509_EXTENSION *ext = X509V3_EXT_conf_nid(
		nullptr, &v3, NID_subject_alt_name, (char *)san);
	X509_add_ext(x, ext, -1);
	X509_EXTENSION_free(ext);
	X509_sign(x, pkey, EVP_sha256());
	return x;
}

/* The address pupnp connects to for a host name (first usable result). */
int loopback_family(const char *host)
{
	struct addrinfo hints;
	struct addrinfo *res = nullptr;
	int family = AF_INET;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, nullptr, &hints, &res) == 0) {
		for (struct addrinfo *r = res; r; r = r->ai_next) {
			if (r->ai_family == AF_INET ||
				r->ai_family == AF_INET6) {
				family = r->ai_family;
				break;
			}
		}
		freeaddrinfo(res);
	}
	return family;
}

/* One-shot TLS server on a loopback address. */
class TlsServer
{
public:
	std::atomic<bool> handshake_ok{false};
	std::atomic<bool> plaintext{false};
	unsigned short port = 0;

	bool start(X509 *cert, EVP_PKEY *pkey, int family = AF_INET)
	{
		family_ = family;
		ctx_ = SSL_CTX_new(TLS_server_method());
		if (!ctx_ || SSL_CTX_use_certificate(ctx_, cert) != 1 ||
			SSL_CTX_use_PrivateKey(ctx_, pkey) != 1)
			return false;
		lfd_ = socket(family, SOCK_STREAM, 0);
		if (lfd_ < 0)
			return false;
		struct sockaddr_storage ss;
		socklen_t len;
		memset(&ss, 0, sizeof(ss));
		if (family == AF_INET6) {
			struct sockaddr_in6 *a = (struct sockaddr_in6 *)&ss;
			a->sin6_family = AF_INET6;
			a->sin6_addr = in6addr_loopback;
			len = sizeof(*a);
		} else {
			struct sockaddr_in *a = (struct sockaddr_in *)&ss;
			a->sin_family = AF_INET;
			a->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			len = sizeof(*a);
		}
		if (bind(lfd_, (struct sockaddr *)&ss, len) != 0 ||
			listen(lfd_, 1) != 0 ||
			getsockname(lfd_, (struct sockaddr *)&ss, &len) != 0)
			return false;
		port = ntohs(family == AF_INET6
				     ? ((struct sockaddr_in6 *)&ss)->sin6_port
				     : ((struct sockaddr_in *)&ss)->sin_port);
		thread_ = std::thread(&TlsServer::serve, this);
		return true;
	}

	/* Wakes the server if the client never connected, then joins it. */
	void stop()
	{
		if (thread_.joinable()) {
			stopping_ = true;
			int fd = socket(family_, SOCK_STREAM, 0);
			struct sockaddr_storage ss;
			socklen_t len = sizeof(ss);
			getsockname(lfd_, (struct sockaddr *)&ss, &len);
			if (connect(fd, (struct sockaddr *)&ss, len) != 0) {
				/* The server already exited. */
			}
			thread_.join();
			close(fd);
		}
		if (lfd_ >= 0)
			close(lfd_);
		lfd_ = -1;
		SSL_CTX_free(ctx_);
		ctx_ = nullptr;
	}

	~TlsServer() { stop(); }

private:
	void serve()
	{
		struct pollfd p = {lfd_, POLLIN, 0};
		if (poll(&p, 1, 10000) != 1)
			return;
		int fd = accept(lfd_, nullptr, nullptr);
		if (fd < 0)
			return;
		if (stopping_) {
			close(fd);
			return;
		}
		struct timeval tv = {5, 0};
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		unsigned char first = 0;
		if (recv(fd, &first, 1, MSG_PEEK) == 1 && first != 0x16)
			plaintext = true; /* not a TLS handshake record */
		SSL *ssl = SSL_new(ctx_);
		SSL_set_fd(ssl, fd);
		if (SSL_accept(ssl) == 1) {
			handshake_ok = true;
			char buf[256];
			while (SSL_read(ssl, buf, sizeof(buf)) > 0) {
			}
		}
		SSL_free(ssl);
		close(fd);
	}

	SSL_CTX *ctx_ = nullptr;
	int lfd_ = -1;
	int family_ = AF_INET;
	std::atomic<bool> stopping_{false};
	std::thread thread_;
};

class SslVerifyTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pkey_ = make_key();
		ASSERT_NE(nullptr, pkey_);
		ASSERT_EQ(UPNP_E_SUCCESS,
			UpnpInitSslContext(0, TLS_client_method()));
	}

	void TearDown() override
	{
		server_.stop();
		UpnpSetSslCtx(nullptr);
		X509_free(cert_);
		EVP_PKEY_free(pkey_);
	}

	void serve(const char *san, int family = AF_INET)
	{
		cert_ = make_cert(pkey_, san);
		ASSERT_NE(nullptr, cert_);
		ASSERT_TRUE(server_.start(cert_, pkey_, family));
	}

	void trust_server_cert()
	{
		X509_STORE *store = SSL_CTX_get_cert_store(UpnpGetSslCtx());
		ASSERT_EQ(1, X509_STORE_add_cert(store, cert_));
	}

	std::string url(const char *host)
	{
		return std::string("https://") + host + ":" +
		       std::to_string(server_.port) + "/";
	}

	int open(const std::string &u)
	{
		void *handle = nullptr;
		int ret = UpnpOpenHttpConnection(u.c_str(), &handle, 5);
		if (handle)
			UpnpCloseHttpConnection(handle);
		server_.stop();
		return ret;
	}

	EVP_PKEY *pkey_ = nullptr;
	X509 *cert_ = nullptr;
	TlsServer server_;
};

} // namespace

TEST_F(SslVerifyTest, UntrustedCertificateIsRejected)
{
	serve("IP:127.0.0.1");
	EXPECT_NE(UPNP_E_SUCCESS, open(url("127.0.0.1")));
	EXPECT_FALSE(server_.handshake_ok);
}

TEST_F(SslVerifyTest, TrustedCertificateForAnotherIpIsRejected)
{
	serve("IP:127.0.0.2");
	trust_server_cert();
	EXPECT_NE(UPNP_E_SUCCESS, open(url("127.0.0.1")));
	EXPECT_FALSE(server_.handshake_ok);
}

TEST_F(SslVerifyTest, TrustedCertificateForAnotherNameIsRejected)
{
	serve("DNS:not-the-host", loopback_family("localhost"));
	trust_server_cert();
	EXPECT_NE(UPNP_E_SUCCESS, open(url("localhost")));
	EXPECT_FALSE(server_.handshake_ok);
}

TEST_F(SslVerifyTest, TrustedCertificateForTheIpIsAccepted)
{
	serve("IP:127.0.0.1");
	trust_server_cert();
	EXPECT_EQ(UPNP_E_SUCCESS, open(url("127.0.0.1")));
	EXPECT_TRUE(server_.handshake_ok);
}

TEST_F(SslVerifyTest, TrustedCertificateForTheNameIsAccepted)
{
	serve("DNS:localhost", loopback_family("localhost"));
	trust_server_cert();
	EXPECT_EQ(UPNP_E_SUCCESS, open(url("localhost")));
	EXPECT_TRUE(server_.handshake_ok);
}

/* An application may opt out of verification on the context it owns. */
TEST_F(SslVerifyTest, ApplicationCanDisableVerification)
{
	serve("IP:127.0.0.2");
	SSL_CTX_set_verify(UpnpGetSslCtx(), SSL_VERIFY_NONE, nullptr);
	EXPECT_EQ(UPNP_E_SUCCESS, open(url("127.0.0.1")));
	EXPECT_TRUE(server_.handshake_ok);
}

/* Paths without TLS must not send an https:// request in clear text. */
TEST_F(SslVerifyTest, DownloadOfHttpsUrlIsNotSentInClear)
{
	serve("IP:127.0.0.1");
	trust_server_cert();
	char *buf = nullptr;
	char content_type[LINE_SIZE];
	int ret = UpnpDownloadUrlItem(
		url("127.0.0.1").c_str(), &buf, content_type);
	free(buf);
	server_.stop();
	EXPECT_FALSE(server_.plaintext);
	EXPECT_EQ(UPNP_E_INVALID_URL, ret);
}

#endif /* UPNP_ENABLE_OPEN_SSL */

int main(int argc, char **argv)
{
#ifdef UPNP_ENABLE_OPEN_SSL
	/* The test server writes to clients that abort the handshake. */
	signal(SIGPIPE, SIG_IGN);
#endif
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
