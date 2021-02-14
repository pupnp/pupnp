CMAKE_MINIMUM_REQUIRED (VERSION 3.11)
CMAKE_POLICY (VERSION 3.13)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include (CmDaB.cmake)

if (NOT PUPNP_VERSION_STRING)
	file (GLOB_RECURSE MACROFILES
		${CMAKE_CURRENT_SOURCE_DIR}/
		*.m4
	)

	list (APPEND MACROFILES ${CMAKE_CURRENT_SOURCE_DIR}/configure.ac)
	list (APPEND WRITTEN_VARS DEBUG)
	list (APPEND WRITTEN_VARS NDEBUG)

	foreach (MACROFILE ${MACROFILES})
		file (STRINGS ${MACROFILE} configure)
		file (REMOVE ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm)

		foreach (line ${configure})
			string (REGEX REPLACE "\\]" "" line ${line})
			string (REGEX REPLACE "\\[" "" line ${line})
			string (REGEX REPLACE ";" "" line ${line})
			string (REGEX REPLACE "[ \t\r\n] *" " " line ${line})

			if (line MATCHES "AC_INIT.* ([0-9]*\\.[0-9]*\\.[0-9]*).*")
				message (STATUS "Setting package-version to ${CMAKE_MATCH_1}")
				set (PUPNP_VERSION_STRING ${CMAKE_MATCH_1} CACHE STRING "Version of the whole package" FORCE)
			elseif (line MATCHES "[. \t]*AC_DEFINE_UNQUOTED *\\(([^,]*), *([^,]*), *([^\\)]*)")
				set (SAVED_MATCH ${CMAKE_MATCH_1})

				if ("${CMAKE_MATCH_1}" IN_LIST WRITTEN_VARS)
					continue()
				endif()

				string (SUBSTRING ${CMAKE_MATCH_2} 0 1 FIRSTCHAR)
				string (STRIP ${CMAKE_MATCH_3} ${CMAKE_MATCH_3})
				file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "/* ${CMAKE_MATCH_3} */\n")

				if (FIRSTCHAR STREQUAL "\"")
					file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine ${CMAKE_MATCH_1} \"\$\{${CMAKE_MATCH_1}\}\"\n\n")
				else()
					if (${CMAKE_MATCH_1} MATCHES VERSION AND NOT ${${CMAKE_MATCH_1}})
						file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine01 ${SAVED_MATCH}\n\n")
					else()
						file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine ${SAVED_MATCH} \$\{${SAVED_MATCH}\}\n\n")
					endif()
				endif()

				list (APPEND WRITTEN_VARS ${SAVED_MATCH})
			elseif (line MATCHES "[. \t]*AC_DEFINE *\\(([^,]*), *([^,]*), *([^\\)]*)")
				if ("${CMAKE_MATCH_1}" IN_LIST WRITTEN_VARS)
					continue()
				endif()

				string (STRIP ${CMAKE_MATCH_3} ${CMAKE_MATCH_3})
				file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "/* ${CMAKE_MATCH_3} */\n")
				file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine ${CMAKE_MATCH_1} 1\n\n")
 				list (APPEND WRITTEN_VARS ${CMAKE_MATCH_1})
			elseif (line MATCHES "^AC_SUBST.*LT_VERSION_IXML, ([0-9]*):([0-9]*):([0-9]*).*")
				set (IXML_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE STRING "Majorversion of libixml" FORCE)
				set (IXML_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3} CACHE STRING "Version of libixml" FORCE)
				message (STATUS "Setting ixml-version to ${IXML_VERSION}")
			elseif (line MATCHES "^AC_SUBST.*LT_VERSION_UPNP, ([0-9]*):([0-9]*):([0-9]*).*")
				set (UPNP_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE STRING "Majorversion of libupnp" FORCE)
				set (UPNP_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3} CACHE STRING "Version of libupnp" FORCE)
				set (UPNP_VERSION_STRING ${UPNP_VERSION} CACHE STRING "see upnpconfig.h" FORCE)
				message (STATUS "Setting upnp-version to ${UPNP_VERSION}")
			endif()
		endforeach()
	endforeach()
endif()

project (PUPNP VERSION ${PUPNP_VERSION_STRING} LANGUAGES C)
include (GNUInstallDirs)

if (WIN32)
	set (CMAKE_DEBUG_POSTFIX d)
	set (STATIC_POSTFIX s)
endif()

option (BUILD_TESTING "Run Tests after compile" ON)

if (BUILD_TESTING)
	enable_testing()
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	set (DEFAULT_BUILD_TYPE "Debug")
endif (EXISTS "${CMAKE_SOURCE_DIR}/.git")

# Set the possible values of build type for cmake-gui
if (CMAKE_CONFIGURATION_TYPES)
	set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE
		STRING "Semicolon separated list of supported configuration types, only supports debug and release, anything else will be ignored" FORCE
	)

	set_property (CACHE CMAKE_CONFIGURATION_TYPES PROPERTY STRINGS
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

option (blocking_tcp_connections "blocking TCP connections" ON)

if (blocking_tcp_connections)
	set (UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS 1) #see upnpconfig.h
endif()

option (scriptsupport "script support for IXML document tree, see ixml.h" ON)
set (IXML_HAVE_SCRIPTSUPPORT ${scriptsupport}) #see upnpconfig.h
option (postwrite "write to the filesystem on otherwise unhandled POST requests" OFF)
set (UPNP_ENABLE_POST_WRITE ${postwrite}) #see upnpconfig.h
option (reuseaddr "bind the miniserver socket with reuseaddr to allow clean restarts" OFF)
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
	set (HAVE_SOCKET_H sys/socket.h)
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
		HAVE_SOCKET_H
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
endif()

if (HAVE_WS2TCPIP_H)
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
if (NOT OFF_T_SIZE)
	check_type_size (off_t OFF_T_SIZE)
	set (UPNP_LARGEFILE_SENSITIVE FALSE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)

	if (OFF_T_SIZE EQUAL 8)
		message (STATUS "System uses 64 bit, no flags needed")
	else()
		unset (OFF_T_SIZE CACHE)
		set (CMAKE_REQUIRED_DEFINITIONS _FILE_OFFSET_BITS=64)
		check_type_size (off_t OFF_T_SIZE)

		if (OFF_T_SIZE EQUAL 8)
			message (STATUS "_FILE_OFFSET_BITS=64 needed")
			set (UPNP_LARGEFILE_SENSITIVE TRUE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)
			set (_FILE_OFFSET_BITS 64 CACHE BOOL "Number of bits in a file offset, on hosts where this is settable" FORCE)
		else()
			unset (OFF_T_SIZE CACHE)
			set (CMAKE_REQUIRED_DEFINITIONS _LARGE_FILES)
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
if (NOT WIN32)
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
	VERSION ${IXML_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file (
	UPNP.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/UPNPConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UPNP
)

write_basic_package_version_file (UPNPConfigVersion.cmake
	VERSION ${UPNP_VERSION}
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

set (VERSION ${UPNP_VERSION_STRING})
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
