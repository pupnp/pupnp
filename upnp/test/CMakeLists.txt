UPNP_Add_Unit_Test(test-upnp-list test_list.c)
UPNP_Add_Unit_Test(test-upnp-init test_init.c)
UPNP_Add_Unit_Test(test-upnp-log test_log.c)
UPNP_Add_Unit_Test(test-upnp-url test_url.c)
UPNP_Add_Unit_Test(test-upnp-sock-io test_sock_io.c)
UPNP_Add_Unit_Test(test-upnp-ssdp-req-port test_ssdp_req_port.c)
# test_threadpool_overflow calls internal ThreadPool API (ThreadPoolInit,
# TPAttrInit, …) that have hidden visibility in the shared library.  Compile
# the three ThreadPool translation units directly into each test variant so
# both shared and static builds test the same code without needing symbol
# exports.
set(TP_OVERFLOW_SOURCES
	test_threadpool_overflow.c
	${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil/ThreadPool.c
	${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil/FreeList.c
	${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil/LinkedList.c
)
set(TP_OVERFLOW_INCLUDES
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc
	PRIVATE ${PUPNP_BINARY_DIR}
)

if(UPNP_BUILD_SHARED)
	add_executable(test-upnp-threadpool-overflow ${TP_OVERFLOW_SOURCES})
	target_include_directories(test-upnp-threadpool-overflow
		${TP_OVERFLOW_INCLUDES})
	target_compile_definitions(test-upnp-threadpool-overflow
		PRIVATE $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>)
	target_link_libraries(test-upnp-threadpool-overflow PRIVATE Threads::Shared)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test-upnp-threadpool-overflow
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.)
	endif()
	add_test(NAME test-upnp-threadpool-overflow
		COMMAND test-upnp-threadpool-overflow)
	if(MSVC OR MSYS OR MINGW OR CYGWIN)
		UPNP_Find_Test_Env(test-upnp-threadpool-overflow TEST_ENV)
		set_tests_properties(test-upnp-threadpool-overflow
			PROPERTIES ENVIRONMENT "${TEST_ENV}")
	endif()
endif()

if(UPNP_BUILD_STATIC)
	add_executable(test-upnp-threadpool-overflow-static ${TP_OVERFLOW_SOURCES})
	target_include_directories(test-upnp-threadpool-overflow-static
		${TP_OVERFLOW_INCLUDES})
	target_compile_definitions(test-upnp-threadpool-overflow-static
		PRIVATE $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>)
	target_link_libraries(test-upnp-threadpool-overflow-static
		PRIVATE Threads::Static)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(test-upnp-threadpool-overflow-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.)
	endif()
	add_test(NAME test-upnp-threadpool-overflow-static
		COMMAND test-upnp-threadpool-overflow-static)
endif()
UPNP_Add_Unit_Test(test-upnp-gena-subscribe-dos test_gena_subscribe_dos.c)
UPNP_Add_Unit_Test(test-upnp-gena-subscribe-pressure test_gena_subscribe_pressure.c)
# Both GENA issue-#435 tests read constants from config.h (a private header).
if(UPNP_BUILD_SHARED)
	target_include_directories(
		test-upnp-gena-subscribe-dos
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
	)
	target_include_directories(
		test-upnp-gena-subscribe-pressure
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
	)
	target_link_libraries(
		test-upnp-gena-subscribe-pressure PRIVATE Threads::Threads
	)
endif()
if(UPNP_BUILD_STATIC)
	target_include_directories(
		test-upnp-gena-subscribe-dos-static
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
	)
	target_include_directories(
		test-upnp-gena-subscribe-pressure-static
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
	)
	target_link_libraries(
		test-upnp-gena-subscribe-pressure-static PRIVATE Threads::Threads
	)
endif()

# On Solaris/illumos, getaddrinfo and freeaddrinfo live in libsocket (and
# sometimes libnsl) rather than in the C library.  find_library() probes for
# each library at configure time and stores the full path (or "<VAR>-NOTFOUND")
# in the named variable.  On Linux, macOS, and other platforms that bundle
# these symbols in libc the probe simply sets the variable to "<VAR>-NOTFOUND",
# which is treated as "not found" by the generator expressions below.
find_library(SOCKET_LIBRARY NAMES socket)
find_library(NSL_LIBRARY NAMES nsl)

# Both GENA issue-#435 tests call socket(), connect(), send(), recv() directly.
# On Solaris/illumos those symbols live in libsocket, not libc; link explicitly.
if(UPNP_BUILD_SHARED)
	target_link_libraries(test-upnp-gena-subscribe-dos PRIVATE
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
	target_link_libraries(test-upnp-gena-subscribe-pressure PRIVATE
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()
if(UPNP_BUILD_STATIC)
	target_link_libraries(test-upnp-gena-subscribe-dos-static PRIVATE
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
	target_link_libraries(test-upnp-gena-subscribe-pressure-static PRIVATE
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()

# Issue #423: crash when socket fd >= FD_SETSIZE due to FD_SET overflow in
# Check_Connect_And_Wait_Connection().  The fix uses poll() on non-Windows.
UPNP_Add_Unit_Test(test-upnp-connect-high-fd test_connect_high_fd.c)
if(UPNP_BUILD_SHARED)
	target_link_libraries(test-upnp-connect-high-fd PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()
if(UPNP_BUILD_STATIC)
	target_link_libraries(test-upnp-connect-high-fd-static PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()

if(UPNP_BUILD_SHARED)
	add_executable(
		test-upnp-parse-uri
		test_parse_uri.c
		test_parse_uri_impl.c
	)
	# Link against the Solaris socket/nsl libraries when they were found.
	#
	# On Windows, Winsock2 functions (htons, getaddrinfo, freeaddrinfo, inet_pton)
	# used in parse_hostport() are not linked automatically. We must explicitly link
	# against ws2_32 to resolve these symbols at link time.
	#
	# The generator expression $<$<BOOL:${VAR}>:${VAR}> works as follows:
	#   - The inner $<BOOL:${VAR}> evaluates to 1 when VAR is a non-empty,
	#     non-false value (i.e. when find_library found the library), and to
	#     0 otherwise (VAR is empty or ends with "-NOTFOUND").
	#   - The outer $<condition:value> expands to 'value' when condition is 1,
	#     and to nothing (an empty string) when condition is 0.
	# Together they add the library path only on platforms that have it.
	target_link_libraries(test-upnp-parse-uri PRIVATE upnp_shared
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
		$<$<PLATFORM_ID:Windows>:ws2_32>
	)
	target_include_directories(
		test-upnp-parse-uri
		PRIVATE ${PUPNP_BINARY_DIR}
		PRIVATE ${PUPNP_BINARY_DIR}/upnp/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ixml/inc
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(
			test-upnp-parse-uri
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.
		)
	endif()
	add_test(NAME test-upnp-parse-uri COMMAND test-upnp-parse-uri)
endif()

if(UPNP_BUILD_STATIC)
	add_executable(
		test-upnp-parse-uri-static
		test_parse_uri.c
		test_parse_uri_impl.c
	)
	target_link_libraries(test-upnp-parse-uri-static PRIVATE upnp_static
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
		$<$<PLATFORM_ID:Windows>:ws2_32>
	)
	target_include_directories(
		test-upnp-parse-uri-static
		PRIVATE ${PUPNP_BINARY_DIR}
		PRIVATE ${PUPNP_BINARY_DIR}/upnp/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/threadutil
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ixml/inc
	)
	if(HAVE_MACRO_PREFIX_MAP)
		target_compile_options(
			test-upnp-parse-uri-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.
		)
	endif()
	add_test(NAME test-upnp-parse-uri-static COMMAND test-upnp-parse-uri-static)
endif()

# Issue #347: HTTP response headers must use title-case names.
# Starts the UPnP HTTP server via UpnpInit2, sends a GET request over a raw
# TCP socket, and verifies title-case header names in the response.
# Skipped on Windows (raw POSIX sockets) and when no network is available.
UPNP_Add_Unit_Test(test-upnp-http-headers test_http_headers.c)
if(UPNP_BUILD_SHARED)
	target_link_libraries(test-upnp-http-headers PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()
if(UPNP_BUILD_STATIC)
	target_link_libraries(test-upnp-http-headers-static PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()
