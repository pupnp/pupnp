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

# ithread_rwlock_t must allow concurrent readers (not serialize them like a
# mutex).  The test exits 77 (skip) when UPNP_USE_RWLOCK=0 (mutex fallback is
# intentional on that platform).
UPNP_Add_Unit_Test(test-upnp-ithread-rwlock test_ithread_rwlock.c)
if(UPNP_BUILD_SHARED)
	target_include_directories(test-upnp-ithread-rwlock
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc)
	target_link_libraries(test-upnp-ithread-rwlock PRIVATE Threads::Threads)
	set_tests_properties(test-upnp-ithread-rwlock
		PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(UPNP_BUILD_STATIC)
	target_include_directories(test-upnp-ithread-rwlock-static
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/inc)
	target_link_libraries(test-upnp-ithread-rwlock-static PRIVATE Threads::Threads)
	set_tests_properties(test-upnp-ithread-rwlock-static
		PROPERTIES SKIP_RETURN_CODE 77)
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

# Issue #227 regression (PR #579): SSDP socket bound to unicast address cannot
# receive multicast M-SEARCH queries on Linux.  Skipped on non-Linux platforms.
UPNP_Add_Unit_Test(test-upnp-ssdp-multicast-recv test_ssdp_multicast_recv.c)
if(UPNP_BUILD_SHARED)
	set_tests_properties(test-upnp-ssdp-multicast-recv
		PROPERTIES SKIP_RETURN_CODE 77)
	target_link_libraries(test-upnp-ssdp-multicast-recv PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()
if(UPNP_BUILD_STATIC)
	set_tests_properties(test-upnp-ssdp-multicast-recv-static
		PROPERTIES SKIP_RETURN_CODE 77)
	target_link_libraries(test-upnp-ssdp-multicast-recv-static PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()

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

# Issue #153: use-after-free in the alias lifecycle when an HTTP handler grabs
# the alias concurrently with unregistration.  Detected by ASan.
# The test calls web_server_ut_* hooks exported from libupnp (extern decls
# in the test file avoid pulling in the internal header chain).
UPNP_Add_Unit_Test(test-upnp-gh-153 poc_gh_153.c)

# Issue #325: crash in alias_release() when UpnpFinish() destroys gWebMutex
# while a thread pool thread still holds or waits on it.  Detected by TSan.
# Uses the same web_server_ut_* hooks and requires Threads::Threads because
# the test spawns a background thread with pthread_create.
UPNP_Add_Unit_Test(test-upnp-gh-325 poc_gh_325.c)
if(UPNP_BUILD_SHARED)
	target_link_libraries(test-upnp-gh-325 PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
endif()
if(UPNP_BUILD_STATIC)
	target_link_libraries(test-upnp-gh-325-static PRIVATE
		Threads::Threads
		$<$<BOOL:${SOCKET_LIBRARY}>:${SOCKET_LIBRARY}>
		$<$<BOOL:${NSL_LIBRARY}>:${NSL_LIBRARY}>
	)
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

# Issue #365: UpnpPrintf must not appear in the shared library's public symbol
# table.  The script exits 77 (skip) when nm is unavailable or on non-ELF
# platforms (Windows, macOS).
if(UPNP_BUILD_SHARED)
	add_test(
		NAME test-upnp-UpnpPrintf-not-exported
		COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/check_symbol_visibility.sh
			$<TARGET_FILE:upnp_shared>
	)
	# Clear LD_PRELOAD so the sh subprocess does not inherit TSan's preload library.
	# TSan CI sets LD_PRELOAD=libtsan.so globally; sh running under TSan with
	# TSAN_OPTIONS=halt_on_error=1 causes a SEGFAULT before the script can run.
	set_tests_properties(test-upnp-UpnpPrintf-not-exported
		PROPERTIES
			SKIP_RETURN_CODE 77
			ENVIRONMENT "LD_PRELOAD=")
endif()

if(NOT UPNP_ENABLE_TESTING_INTEGRATION)
	return()
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/find_package_test.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/findpackage_test/CMakeLists.txt @ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/find_package_no_components.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/findpackage_no_components_test/CMakeLists.txt
	@ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/find_package_wrong_components.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test/CMakeLists.txt
	@ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test/CMakeLists.txt @ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory_exclude.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test/CMakeLists.txt
	@ONLY)

add_test(NAME upnp_install
	COMMAND ${CMAKE_COMMAND}
		--install ${PUPNP_BINARY_DIR}
		--prefix ${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		--config $<CONFIG>
    WORKING_DIRECTORY ${PUPNP_BINARY_DIR})

set_tests_properties(upnp_install
	PROPERTIES FIXTURES_SETUP upnp_install)

# CMAKE_GENERATOR_PLATFORM doesn't work in the if
set(GENERATOR ${CMAKE_GENERATOR_PLATFORM})

if(GENERATOR)
	set(PLATFORM "-A ${GENERATOR}")
endif(GENERATOR)

#
# findpackage_test
#
add_test(NAME upnp_find_package_configure
	COMMAND ${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/findpackage_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		-DUPNP_ROOT=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_test)

add_test(NAME upnp_find_package_build
	COMMAND ${CMAKE_COMMAND}
		--build .
		--config $<CONFIG>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/findpackage_test_build)

set_tests_properties(upnp_find_package_configure
	PROPERTIES
		FIXTURES_REQUIRED upnp_install
		FIXTURES_SETUP upnp_fp_config)

set_tests_properties(upnp_find_package_build
	PROPERTIES
		FIXTURES_REQUIRED upnp_fp_config
		FIXTURES_SETUP upnp_fp_build)

#
# findpackage_no_components_test
#
add_test(NAME upnp_find_package_no_components_configure
	COMMAND ${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/findpackage_no_components_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		-DUPNP_ROOT=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_no_components_test)

set_tests_properties(upnp_find_package_no_components_configure
	PROPERTIES
		FIXTURES_REQUIRED upnp_install)

#
# findpackage_wrong_component_test
#
add_test(NAME upnp_find_package_wrong_components_configure
	COMMAND	${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		-DUPNP_ROOT=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test)

set_tests_properties(upnp_find_package_wrong_components_configure
	PROPERTIES
		FIXTURES_REQUIRED upnp_install
		WILL_FAIL TRUE)

#
# add_subdirectory_test_shared
#
if(UPNP_BUILD_SHARED)
	add_test(NAME upnp_add_subdirectory_configure_shared
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_shared
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_SHARED=ON
			-DUPNP_BUILD_STATIC=OFF
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test)

	add_test(NAME upnp_add_subdirectory_build_shared
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_shared)

	set_tests_properties(upnp_add_subdirectory_configure_shared
		PROPERTIES
			FIXTURES_REQUIRED upnp_install
			FIXTURES_SETUP upnp_as_config_shared)

	set_tests_properties(upnp_add_subdirectory_build_shared
		PROPERTIES
		FIXTURES_REQUIRED upnp_as_config_shared)
endif()

#
# add_subdirectory_test_static
#
if(UPNP_BUILD_STATIC)
	add_test(NAME upnp_add_subdirectory_configure_static
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_static
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_STATIC=ON
			-DUPNP_BUILD_SHARED=OFF
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test)

	add_test(NAME upnp_add_subdirectory_build_static
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_static)

	set_tests_properties(upnp_add_subdirectory_configure_static
		PROPERTIES
			FIXTURES_REQUIRED upnp_install
			FIXTURES_SETUP upnp_as_config_static)

	set_tests_properties(upnp_add_subdirectory_build_static
		PROPERTIES
		FIXTURES_REQUIRED upnp_as_config_static)
endif()

#
# add_subdirectory_exclude_test_shared
#
if(UPNP_BUILD_SHARED)
	add_test(NAME upnp_add_subdirectory_exclude_configure_shared
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_shared
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_SHARED=ON
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test)

	add_test(NAME upnp_add_subdirectory_exclude_build_shared
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY
			${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_shared)


	set_tests_properties(upnp_add_subdirectory_exclude_configure_shared
		PROPERTIES
			FIXTURES_REQUIRED upnp_install
			FIXTURES_SETUP upnp_asx_config_shared)

	set_tests_properties(upnp_add_subdirectory_exclude_build_shared
		PROPERTIES
			FIXTURES_REQUIRED upnp_asx_config_shared)
endif()

#
# add_subdirectory_exclude_test_static
#
if(UPNP_BUILD_STATIC)
	add_test(NAME upnp_add_subdirectory_exclude_configure_static
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_static
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/upnp_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_STATIC=ON
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test)

	add_test(NAME upnp_add_subdirectory_exclude_build_static
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY
			${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_static)


	set_tests_properties(upnp_add_subdirectory_exclude_configure_static
		PROPERTIES
			FIXTURES_REQUIRED upnp_install
			FIXTURES_SETUP upnp_asx_config_static)

	set_tests_properties(upnp_add_subdirectory_exclude_build_static
		PROPERTIES
			FIXTURES_REQUIRED upnp_asx_config_static)
endif()
