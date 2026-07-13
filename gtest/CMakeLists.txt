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

# regression test for issue #379 — gena_validate_delivery_urls() must accept
# cross-subnet delivery URLs when both device and subscriber are on RFC 1918
# private addresses; accesses internal symbols so only the static variant.
if (NOT WIN32 AND UPNP_BUILD_STATIC)
	add_executable(test_gena-static test_gena.cpp)
	target_link_libraries(test_gena-static PRIVATE upnp_static GTest::gtest)
	target_include_directories(test_gena-static PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/inc/
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test_gena-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()
	gtest_add_tests(
		TARGET test_gena-static
		TEST_PREFIX test-upnp-
		TEST_SUFFIX -static
	)
endif()

# regression test — Windows UpnpGetIfInfo() must not format an address
# family that was not found on the interface (amule-org/amule#242, #301);
# UpnpSetIfAddrStrings() is an internal symbol so only the static variant
# is built.
if (NOT WIN32 AND UPNP_BUILD_STATIC)
	add_executable(test_upnpapi_setifaddrstrings-static
		test_upnpapi_setifaddrstrings.cpp)
	target_link_libraries(test_upnpapi_setifaddrstrings-static
		PRIVATE upnp_static GTest::gtest)
	target_include_directories(test_upnpapi_setifaddrstrings-static PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/inc/
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test_upnpapi_setifaddrstrings-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()
	gtest_add_tests(
		TARGET test_upnpapi_setifaddrstrings-static
		TEST_PREFIX test-upnp-
		TEST_SUFFIX -static
	)
endif()

# regression test for issue #395 — sock_write must not pass MSG_DONTROUTE to
# send() on TCP sockets; sock_write() is an internal symbol (hidden visibility
# in the shared library) so only the static variant is built.
if (NOT WIN32 AND UPNP_BUILD_STATIC)
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

# regression test for issue #394 — http_RecvMessage must reject oversized POST
# bodies before buffering them; accesses internal symbols so static only.
if (NOT WIN32 AND UPNP_BUILD_STATIC)
	add_executable(test_httpreadwrite-static test_httpreadwrite.cpp)
	target_link_libraries(test_httpreadwrite-static PRIVATE upnp_static GTest::gtest)
	target_include_directories(test_httpreadwrite-static PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/inc/
		${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test_httpreadwrite-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()
	gtest_add_tests(
		TARGET test_httpreadwrite-static
		TEST_PREFIX test-upnp-
		TEST_SUFFIX -static
	)
endif()
