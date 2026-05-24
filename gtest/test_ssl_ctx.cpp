// regression: issue #169

#include "upnp.h"

#include "gtest/gtest.h"

#ifdef UPNP_ENABLE_OPEN_SSL

class SslCtxTest : public ::testing::Test
{
protected:
	void TearDown() override { UpnpSetSslCtx(nullptr); }
};

TEST_F(SslCtxTest, GetReturnsNullBeforeInit)
{
	EXPECT_EQ(nullptr, UpnpGetSslCtx());
}

TEST_F(SslCtxTest, GetReturnsNonNullAfterInit)
{
	ASSERT_EQ(UPNP_E_SUCCESS, UpnpInitSslContext(0, TLS_client_method()));
	EXPECT_NE(nullptr, UpnpGetSslCtx());
}

TEST_F(SslCtxTest, SetSucceedsWhenNoPriorContext)
{
	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
	ASSERT_NE(nullptr, ctx);
	EXPECT_EQ(UPNP_E_SUCCESS, UpnpSetSslCtx(ctx));
	EXPECT_EQ(ctx, UpnpGetSslCtx());
}

TEST_F(SslCtxTest, SetFailsWhenAlreadySet)
{
	ASSERT_EQ(UPNP_E_SUCCESS, UpnpInitSslContext(0, TLS_client_method()));
	SSL_CTX *ctx2 = SSL_CTX_new(TLS_client_method());
	ASSERT_NE(nullptr, ctx2);
	EXPECT_EQ(UPNP_E_INIT, UpnpSetSslCtx(ctx2));
	SSL_CTX_free(ctx2);
}

TEST_F(SslCtxTest, SetNullClearsContext)
{
	ASSERT_EQ(UPNP_E_SUCCESS, UpnpInitSslContext(0, TLS_client_method()));
	ASSERT_NE(nullptr, UpnpGetSslCtx());
	EXPECT_EQ(UPNP_E_SUCCESS, UpnpSetSslCtx(nullptr));
	EXPECT_EQ(nullptr, UpnpGetSslCtx());
}

#endif /* UPNP_ENABLE_OPEN_SSL */

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
