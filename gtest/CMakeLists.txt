include (GoogleTest)

UPNP_addGTest (test_template test_template.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

if(UPNP_ENABLE_OPEN_SSL)
	UPNP_addGTest(test_ssl_ctx test_ssl_ctx.cpp)
endif()

#UPNP_addGTest (test_UpnpHttpHeaderList test_UpnpHttpHeaderList.cpp
#	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
#)

#if (NOT WIN32)
#	UPNP_addGTest (test_upnpapi test_upnpapi.cpp
#		ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
#	)
#endif()

# regression test for issue #195 — accesses internal symbols (gIF_IPV6 etc.)
# that have hidden visibility in the shared library, so only the static variant
# is built.
if (NOT WIN32 AND UPNP_BUILD_STATIC)
	include(GoogleTest)
	add_executable(test_miniserver-static test_miniserver.cpp)
	target_link_libraries(test_miniserver-static PRIVATE upnp_static GTest::gmock)
	target_include_directories(test_miniserver-static PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/inc/
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test_miniserver-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()
	gtest_add_tests(
		TARGET test_miniserver-static
		TEST_PREFIX test-upnp-
		TEST_SUFFIX -static
	)
endif()

# regression test for issue #395 — sock_write must not pass MSG_DONTROUTE to
# send() on TCP sockets; sock_write() is an internal symbol (hidden visibility
# in the shared library) so only the static variant is built.
if (NOT WIN32 AND UPNP_BUILD_STATIC)
	include(GoogleTest)
	add_executable(test_sock-static test_sock.cpp)
	target_link_libraries(test_sock-static PRIVATE upnp_static GTest::gtest)
	target_include_directories(test_sock-static PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/inc/
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test_sock-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()
	gtest_add_tests(
		TARGET test_sock-static
		TEST_PREFIX test-upnp-
		TEST_SUFFIX -static
	)
endif()
