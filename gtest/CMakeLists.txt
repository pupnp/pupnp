include (GoogleTest)

add_executable (test-upnp-upnpapi
	test_upnpapi.cpp
)
target_include_directories (test-upnp-upnpapi
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

target_link_libraries (test-upnp-upnpapi
	PRIVATE GTest::gmock
	PRIVATE UPNP::Shared
)

gtest_discover_tests (test-upnp-upnpapi
	TEST_PREFIX test-upnp-
)

add_executable (test-upnp-upnpapi-static
	test_upnpapi.cpp
)

target_include_directories (test-upnp-upnpapi-static
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

target_link_libraries (test-upnp-upnpapi-static
	PRIVATE GTest::gmock
	PRIVATE UPNP::Static
)

gtest_discover_tests (test-upnp-upnpapi-static
	TEST_PREFIX test-upnp-
	TEST_SUFFIX -static
)
