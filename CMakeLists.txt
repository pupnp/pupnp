CMAKE_MINIMUM_REQUIRED (VERSION 3.11)
CMAKE_POLICY (VERSION 3.13)

# C++11 is required
# Set here before we create any targets
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (cmake/CmDaB.cmake)
include (cmake/test-functions.cmake)
include (cmake/autoheader.cmake)

project (PUPNP VERSION ${PUPNP_VERSION_STRING})

include (GNUInstallDirs)

# Only set library postfix with MSVC toolchain
if (MSVC)
	set (CMAKE_DEBUG_POSTFIX d)
	set (STATIC_POSTFIX s)
endif()

option (UPNP_BUILD_SHARED "Build shared libraries" ON)
option (UPNP_BUILD_STATIC "Build static libraries" ON)
option (BUILD_TESTING "Run Tests after compile" ON)


if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	set (DEFAULT_BUILD_TYPE "Debug")
endif (EXISTS "${CMAKE_SOURCE_DIR}/.git")

# Set the possible values of build type for cmake-gui
if (CMAKE_CONFIGURATION_TYPES)
	set (CMAKE_CONFIGURATION_TYPES "Debug;Release"
		CACHE STRING
		"Semicolon separated list of supported configuration types, only supports debug and release, anything else will be ignored"
		FORCE
	)

	set_property (CACHE CMAKE_CONFIGURATION_TYPES
		PROPERTY STRINGS
		"Debug" "Release"
	)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message (STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")

	set (CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
		STRING "Choose the type of build." FORCE
	)
endif()

#
# Check for libupnp subsets
#
option (client "control point code (client)" ON)

if (client)
	set (UPNP_HAVE_CLIENT 1) #see upnpconfig.h
endif()

option (device "device specific code (implies --disable-webserver if disabled)" ON)

if (device)
	set (UPNP_HAVE_DEVICE 1) #see upnpconfig.h
endif()

include (CMakeDependentOption)
cmake_dependent_option (webserver "integrated web server" ON NOT device OFF)
set (UPNP_HAVE_WEBSERVER ${webserver}) #see upnpconfig.h

option (ssdp "SSDP part" ON)

if (ssdp)
	set (UPNP_HAVE_SSDP 1) #see upnpconfig.h
endif()

option (optssdp "optional SSDP headers support" ON)

if (optssdp)
	set (UPNP_HAVE_OPTSSDP 1) #see upnpconfig.h
endif()

option (soap "SOAP part" ON)

if (soap)
	set (UPNP_HAVE_SOAP 1) #see upnpconfig.h
endif()

option (gena "GENA part" ON)

if (gena)
	set (UPNP_HAVE_GENA 1) #see upnpconfig.h
endif()

if (gena OR optssdp)
	set (uuid TRUE)
endif()

option (tools "helper APIs in upnptools.h" ON)

if (tools)
	set (UPNP_HAVE_TOOLS 1) #see upnpconfig.h
endif()

option (ipv6 "ipv6 support" ON)

if (ipv6)
	set (UPNP_ENABLE_IPV6 1) #see upnpconfig.h
endif()

option (unspecified_server "unspecified SERVER header" OFF)
set (UPNP_ENABLE_UNSPECIFIED_SERVER ${unspecified_server}) #see upnpconfig.h
option (open_ssl "open-ssl support" OFF)

if (open_ssl)
	include (FindOpenSSL)

	if (OPENSSL_FOUND)
		set (UPNP_ENABLE_OPEN_SSL 1) #see upnpconfig.h
	else()
		message (FATAL_ERROR "openssl not found")
	endif()
endif()

option (blocking_tcp_connections "blocking TCP connections" OFF)
if (blocking_tcp_connections)
	set (UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS 1) #see upnpconfig.h
endif()

option (scriptsupport "script support for IXML document tree, see ixml.h" ON)
set (IXML_HAVE_SCRIPTSUPPORT ${scriptsupport}) #see upnpconfig.h

option (reuseaddr "Bind the miniserver socket with SO_REUSEADDR to allow clean restarts" ON)
set (UPNP_MINISERVER_REUSEADDR ${reuseaddr}) #see upnpconfig.h

option (samples "compilation of upnp/sample/ code" ON)

find_package (Git)
cmake_dependent_option (DOWNLOAD_AND_BUILD_DEPS "Get all missing stuff" OFF ${Git_FOUND} OFF)

#
# Checks for header files (which aren't needed on Win32)
#
include (CheckIncludeFile)

if (NOT WIN32)
	set (HAVE_INET_H arpa/inet.h)
	set (HAVE_FCNTL_H fcntl.h)
	set (HAVE_INTTYPES_H inttypes.h)
	set (HAVE_LIMITS_H limits.h)
	set (HAVE_NETDB_H netdb.h)
	set (HAVE_IN_H netinet/in.h)
	set (HAVE_STDLIB_H stdlib.h)
	set (HAVE_STRING_H string.h)
	set (HAVE_IOCTL_H sys/ioctl.h)
	set (HAVE_TIME_H sys/time.h)
	set (HAVE_SYSLOG_H syslog.h)
	set (HAVE_UNISTD_H unistd.h)

	set (headers
		HAVE_INET_H
		HAVE_FCNTL_H
		HAVE_INTTYPES_H
		HAVE_LIMITS_H
		HAVE_NETDB_H
		HAVE_IN_H
		HAVE_STDLIB_H
		HAVE_STRING_H
		HAVE_IOCTL_H
		HAVE_TIME_H
		HAVE_SYSLOG_H
		HAVE_UNISTD_H
	)

	foreach (header ${headers})
		check_include_file (${${header}} ${header})

		if (NOT ${header})
			message (FATAL_ERROR "Header-file ${${header}} not found")
		endif()
	endforeach()
endif (NOT WIN32)

#
# Checks for typedefs, structures, and compiler characteristics
#
include (TestBigEndian)
test_big_endian (big_endian)
check_include_file (sys/socket.h HAVE_SOCKET_H)
check_include_file (ws2tcpip.h HAVE_WS2TCPIP_H)

if (HAVE_SOCKET_H)
	list (APPEND CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
elseif (HAVE_WS2TCPIP_H)
	list (APPEND CMAKE_EXTRA_INCLUDE_FILES ws2tcpip.h)
endif()

include (CheckTypeSize)
check_type_size (socklen_t SOCKLEN_T)
unset (CMAKE_EXTRA_INCLUDE_FILES)

if (NOT SOCKLEN_T)
	set (socklen_t "int")
endif()
#
# Checks for large-file-sensitivity
#
if (NOT HAVE_OFF_T_SIZE)
	check_type_size (off_t OFF_T_SIZE)
	set (UPNP_LARGEFILE_SENSITIVE FALSE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)

	if (OFF_T_SIZE EQUAL 8)
		message (STATUS "System uses 64 bit, no flags needed")
	else()
		unset (HAVE_OFF_T_SIZE CACHE)
		set (CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64)
		check_type_size (off_t OFF_T_SIZE)

		if (OFF_T_SIZE EQUAL 8)
			message (STATUS "_FILE_OFFSET_BITS=64 needed")
			set (UPNP_LARGEFILE_SENSITIVE TRUE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)
			set (_FILE_OFFSET_BITS 64 CACHE BOOL "Number of bits in a file offset, on hosts where this is settable" FORCE)
		else()
			unset (HAVE_OFF_T_SIZE CACHE)
			set (CMAKE_REQUIRED_DEFINITIONS -D_LARGE_FILES)
			check_type_size (off_t OFF_T_SIZE)

			if (OFF_T_SIZE EQUAL 8)
				message (STATUS "_LARGE_FILES needed")
				set (_LARGE_FILES TRUE CACHE BOOL "Define for large files, on AIX-style hosts." FORCE)
				set (UPNP_LARGEFILE_SENSITIVE TRUE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)
			endif()
		endif()
	endif()
endif()

unset (CMAKE_REQUIRED_DEFINITIONS)
#
# Checks for library functions
#
include (CheckFunctionExists)
check_function_exists (fseeko HAVE_FSEEKO)

if (NOT HAVE_FSEEKO)
	set (CMAKE_REQUIRED_DEFINITIONS _LARGEFILE_SOURCE)
	check_function_exists (fseeko HAVE_FSEEKO)
	unset (CMAKE_REQUIRED_DEFINITIONS)

	if (HAVE_FSEEKO)
		set (_LARGEFILE_SOURCE TRUE CACHE BOOL "Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2)." FORCE)
	endif()
endif()

check_function_exists (strnlen HAVE_STRNLEN)
check_function_exists (strndup HAVE_STRNDUP)

include(CheckCCompilerFlag)
check_c_compiler_flag(-fmacro-prefix-map=from=to HAVE_MACRO_PREFIX_MAP)

if (Solaris)
	set (CMAKE_REQUIRED_LIBRARIES socket)
	check_function_exists (bind HAVE_SOCKET)
	set (CMAKE_REQUIRED_LIBRARIES nsl)
	check_function_exists (gethostbyname HAVE_NSL])
	set (CMAKE_REQUIRED_LIBRARIES rt)
	check_function_exists (sched_getparam HAVE_RT)
	unset (CMAKE_REQUIRED_LIBRARIES)
endif()
#
# Checks for POSIX Threads
#
if (NOT MSVC)
	set (THREADS_PREFER_PTHREAD_FLAG TRUE)
	include (FindThreads)
	if (NOT DOWNLOAD_AND_BUILD_DEPS)
		if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
			add_library (Threads::Shared ALIAS Threads::Threads)
			add_library (Threads::Static ALIAS Threads::Threads)
		else ()
			add_library (Threads::Shared INTERFACE IMPORTED)
			add_library (Threads::Static INTERFACE IMPORTED)

			# The following two blocks replicate the original FindThreads
			if (THREADS_HAVE_PTHREAD_ARG)
				set_property (TARGET Threads::Shared PROPERTY
					INTERFACE_COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CUDA>:SHELL:-Xcompiler -pthread>"
												"$<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:-pthread>"
				)

				set_property (TARGET Threads::Static PROPERTY
					INTERFACE_COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CUDA>:SHELL:-Xcompiler -pthread>"
												"$<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:-pthread>"
				)
			endif()

			if (CMAKE_THREAD_LIBS_INIT)
				get_target_property (thread_location Threads::Threads INTERFACE_LINK_LIBRARIES)

				set_target_properties(Threads::Shared PROPERTIES
					INTERFACE_LINK_LIBRARIES ${thread_location}
				)

				set_target_properties(Threads::Static PROPERTIES
					INTERFACE_LINK_LIBRARIES ${thread_location}
				)
			endif()
		endif()
	endif()
else()
	if (NOT PTHREADS4W_DIR)
		set (PTHREADS4W_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/)
	endif()

	if (NOT DOWNLOAD_AND_BUILD_DEPS)
		find_package (PTHREADS4W CONFIG REQUIRED)
	else()
		find_package (PTHREADS4W CONFIG)

		if (NOT PTHREADS4W_CONFIG)
			CmDaB_install (pthreads4w)
		endif()
	endif()
endif()
#
# Determine if pthread_rwlock_t is available
#
if (TARGET Threads::Threads)
	set (CMAKE_EXTRA_INCLUDE_FILES pthread.h)

	if (DOWNLOAD_AND_BUILD_DEPS AND NOT PTHREADS4W_DIR)
		set (CMAKE_REQUIRED_INCLUDES ${PTHREADS4W_SOURCE_DIR})
	else()
		if (NOT Threads_FOUND)
			get_target_property (CMAKE_REQUIRED_INCLUDES Threads::Threads INTERFACE_INCLUDE_DIRECTORIES)
		endif()
	endif()

	check_type_size (pthread_rwlock_t UPNP_USE_RWLOCK)
	unset (CMAKE_EXTRA_INCLUDE_FILES)
	unset (CMAKE_REQUIRED_INCLUDES)
endif()

if (open_ssl)
	include (FindOpenSSL)

	if (NOT OPENSSL_FOUND)
		message (FATAL_ERROR "openssl not found")
	endif()
endif()

configure_file (${PUPNP_SOURCE_DIR}/upnp/inc/upnpconfig.h.cm ${PUPNP_BINARY_DIR}/upnp/inc/upnpconfig.h)
configure_file (${PUPNP_SOURCE_DIR}/upnp/sample/common/config_sample.h.cm ${PUPNP_BINARY_DIR}/upnp/sample/common/config_sample.h)
configure_file (${PUPNP_BINARY_DIR}/autoconfig.h.cm ${PUPNP_BINARY_DIR}/autoconfig.h)

add_subdirectory (ixml)
add_subdirectory (upnp)

install (EXPORT UPNP
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UPNP
)

include (CMakePackageConfigHelpers)

configure_package_config_file (
	IXML.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/IXMLConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IXML
)

write_basic_package_version_file (IXMLConfigVersion.cmake
	VERSION ${IXML_VERSION_STRING}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file (
	UPNP.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/UPNPConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UPNP
)

write_basic_package_version_file (UPNPConfigVersion.cmake
	VERSION ${UPNP_VERSION_STRING}
	COMPATIBILITY SameMajorVersion
)

install (FILES
	${CMAKE_CURRENT_BINARY_DIR}/IXMLConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/IXMLConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IXML/
)

install (FILES
	${CMAKE_CURRENT_BINARY_DIR}/UPNPConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/UPNPConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UPNP/
)

set (VERSION ${PUPNP_VERSION_STRING})
set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix "\${prefix}")
set (libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
set (includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set (PTHREAD_CFLAGS ${CMAKE_THREAD_LIBS_INIT})

if (UPNP_ENABLE_OPEN_SSL)
	set (OPENSSL_LIBS "-lssl")
endif()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/libupnp.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libupnp.pc @ONLY)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libupnp.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

if (BUILD_TESTING)
	enable_testing()

	#find_package (GTest CONFIG)

	#if (GTest_FOUND)
		#add_subdirectory (gtest)
	#elseif (DOWNLOAD_AND_BUILD_DEPS AND NOT GTest_FOUND)
		#CmDaB_install (googletest)
		#add_subdirectory (gtest)
	#endif()
endif()
