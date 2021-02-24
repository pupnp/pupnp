add_executable (upnp-test-upnpapi
	test_upnpapi.cpp
)

target_link_libraries (upnp-test-upnpapi
	PRIVATE GTest::gtest
	PRIVATE UPNP::Shared
)

gtest_discover_tests (upnp-test-upnpapi
	TEST_PREFIX test-upnp-
)

add_executable (upnp-test-upnpapi-static
	test_upnpapi.cpp
)

target_link_libraries (upnp-test-upnpapi-static
	PRIVATE GTest::gtest
	PRIVATE UPNP::Static
)

gtest_discover_tests (upnp-test-upnpapi-static
	TEST_PREFIX test-upnp-
	TEST_SUFFIX -static
)
