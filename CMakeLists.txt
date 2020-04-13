CMAKE_MINIMUM_REQUIRED (VERSION 3.11)

IF (NOT UPNP_VERSION_STRING)
	FILE (GLOB_RECURSE MACROFILES
		${CMAKE_CURRENT_SOURCE_DIR}/
		*.m4
	)

	LIST (APPEND MACROFILES ${CMAKE_CURRENT_SOURCE_DIR}/configure.ac)

	FOREACH (MACROFILE ${MACROFILES})
		FILE (STRINGS ${MACROFILE} configure)

		FOREACH (line ${configure})
			STRING (REGEX REPLACE "\\]" "" line ${line})
			STRING (REGEX REPLACE "\\[" "" line ${line})
			STRING (REGEX REPLACE ";" "" line ${line})
			STRING (REGEX REPLACE "[ \t\r\n] *" " " line ${line})

			IF (line MATCHES "AC_INIT.* ([0-9]*\\.[0-9]*\\.[0-9]*).*")
				MESSAGE (STATUS "Setting package-version to ${CMAKE_MATCH_1}")
				SET (UPNP_VERSION_STRING ${CMAKE_MATCH_1} CACHE STRING "Version of the whole package" FORCE)
			ELSEIF (line MATCHES "[. \t]*AC_DEFINE_UNQUOTED *\\(([^,]*), *([^,]*), *([^\\)]*)")
				IF ("${CMAKE_MATCH_1}" IN_LIST WRITTEN_VARS)
					CONTINUE()
				ENDIF ("${CMAKE_MATCH_1}" IN_LIST WRITTEN_VARS)

				STRING (SUBSTRING ${CMAKE_MATCH_2} 0 1 FIRSTCHAR)
				STRING (STRIP ${CMAKE_MATCH_3} ${CMAKE_MATCH_3})
				FILE (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "/* ${CMAKE_MATCH_3} */\n")

				IF (FIRSTCHAR STREQUAL "\"")
					FILE (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine ${CMAKE_MATCH_1} \"\$\{${CMAKE_MATCH_1}\}\"\n\n")
				ELSE (FIRSTCHAR STREQUAL "\"")
					IF (${CMAKE_MATCH_1} MATCHES VERSION AND NOT ${${CMAKE_MATCH_1}})
						FILE (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine01 ${CMAKE_MATCH_1}\n\n")
					ELSE (${CMAKE_MATCH_1} MATCHES VERSION AND NOT ${${CMAKE_MATCH_1}})
						FILE (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine ${CMAKE_MATCH_1} \$\{${CMAKE_MATCH_1}\}\n\n")
					ENDIF (${CMAKE_MATCH_1} MATCHES VERSION AND NOT ${${CMAKE_MATCH_1}})
				ENDIF (FIRSTCHAR STREQUAL "\"")

				LIST (APPEND WRITTEN_VARS ${CMAKE_MATCH_1})
			ELSEIF (line MATCHES "[. \t]*AC_DEFINE *\\(([^,]*), *([^,]*), *([^\\)]*)")
				IF ("${CMAKE_MATCH_1}" IN_LIST WRITTEN_VARS)
					CONTINUE()
				ENDIF ("${CMAKE_MATCH_1}" IN_LIST WRITTEN_VARS)

				STRING (STRIP ${CMAKE_MATCH_3} ${CMAKE_MATCH_3})
				FILE (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "/* ${CMAKE_MATCH_3} */\n")
				FILE (APPEND ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm "#cmakedefine ${CMAKE_MATCH_1} 1\n\n")
 				LIST (APPEND WRITTEN_VARS ${CMAKE_MATCH_1})
			ELSEIF (line MATCHES "^AC_SUBST.*LT_VERSION_IXML, ([0-9]*):([0-9]*):([0-9]*).*")
				SET (IXML_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE STRING "Majorversion of libixml" FORCE)
				SET (IXML_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3} CACHE STRING "Version of libixml" FORCE)
				MESSAGE (STATUS "Setting ixml-version to ${IXML_VERSION}")
			ELSEIF (line MATCHES "^AC_SUBST.*LT_VERSION_UPNP, ([0-9]*):([0-9]*):([0-9]*).*")
				STRING (REGEX REPLACE ":" "." TMP_VERSION ${CMAKE_MATCH_1})
				SET (UPNP_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE STRING "Majorversion of libupnp" FORCE)
				SET (UPNP_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3} CACHE STRING "Version of libupnp" FORCE)
				MESSAGE (STATUS "Setting upnp-version to ${UPNP_VERSION}")
			ENDIF (line MATCHES "AC_INIT.* ([0-9]*\\.[0-9]*\\.[0-9]*).*")
		ENDFOREACH (line ${configure})
	ENDFOREACH (MACROFILE ${MACROFILES})
ENDIF (NOT UPNP_VERSION_STRING)

PROJECT (UPNP VERSION ${UPNP_VERSION_STRING})

IF (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	SET (DEFAULT_BUILD_TYPE "Debug")
ENDIF (EXISTS "${CMAKE_SOURCE_DIR}/.git")

# Set the possible values of build type for cmake-gui
IF (CMAKE_CONFIGURATION_TYPES)
	SET_PROPERTY (CACHE CMAKE_CONFIGURATION_TYPES PROPERTY STRINGS
		"Debug" "Release"
	)
ENDIF (CMAKE_CONFIGURATION_TYPES)

IF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	MESSAGE (STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
	SET (CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
		STRING "Choose the type of build." FORCE
	)
ENDIF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

#
# Check for DEBUG flag
#
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
	OPTION (debug "extra debugging code" ON)
ELSE (CMAKE_BUILD_TYPE STREQUAL "Debug")
	OPTION (debug "extra debugging code" OFF)
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

IF (debug)
	SET (UPNP_HAVE_DEBUG 1) #see upnpconfig.h
ENDIF (debug)

IF (debug)
	ADD_COMPILE_DEFINITIONS (DEBUG)
	SET (DEBUG 1) #Define to 1 to compile debug code
ELSE (debug)
	ADD_COMPILE_DEFINITIONS (NDEBUG)
	SET (NDEBUG 1) #Define to 1 to prevent compilation of assert()
ENDIF (debug)

#
# Check for libupnp subsets
#
OPTION (client "control point code (client)" ON)

IF (client)
	SET (UPNP_HAVE_CLIENT 1) #see upnpconfig.h
ENDIF (client)

OPTION (device "device specific code (implies --disable-webserver if disabled)" ON)

IF (device)
	SET (UPNP_HAVE_DEVICE 1) #see upnpconfig.h
ENDIF (device)

INCLUDE (CMakeDependentOption)
CMAKE_DEPENDENT_OPTION (webserver "integrated web server" ON NOT device OFF)
SET (UPNP_HAVE_WEBSERVER ${webserver}) #see upnpconfig.h

OPTION (ssdp "SSDP part" ON)

IF (ssdp)
	SET (UPNP_HAVE_SSDP 1) #see upnpconfig.h
ENDIF (ssdp)

OPTION (optssdp "optional SSDP headers support" ON)

IF (optssdp)
	SET (UPNP_HAVE_OPTSSDP 1) #see upnpconfig.h
ENDIF (optssdp)

OPTION (soap "SOAP part" ON)

IF (soap)
	SET (UPNP_HAVE_SOAP 1) #see upnpconfig.h
ENDIF (soap)

OPTION (gena "GENA part" ON)

IF (gena)
	SET (UPNP_HAVE_GENA 1) #see upnpconfig.h
ENDIF (gena)

IF (gena OR optssdp)
	SET (uuid TRUE)
ENDIF (gena OR optssdp)

OPTION (tools "helper APIs in upnptools.h" ON)

IF (tools)
	SET (UPNP_HAVE_TOOLS 1) #see upnpconfig.h
ENDIF (tools)

OPTION (ipv6 "ipv6 support" ON)

IF (ipv6)
	SET (UPNP_ENABLE_IPV6 1) #see upnpconfig.h
ENDIF (ipv6)

OPTION (unspecified_server "unspecified SERVER header" OFF)
SET (UPNP_ENABLE_UNSPECIFIED_SERVER ${unspecified_server}) #see upnpconfig.h

OPTION (open_ssl "open-ssl support" OFF)

IF (open_ssl)
	INCLUDE (FindOpenSSL)

	IF (OPENSSL_FOUND)
		SET (UPNP_ENABLE_OPEN_SSL 1) #see upnpconfig.h
	ELSE (OPENSSL_FOUND)
		MESSAGE (FATAL_ERROR "openssl not found")
	ENDIF (OPENSSL_FOUND)
ENDIF (open_ssl)

OPTION (blocking_tcp_connections "blocking TCP connections" ON)

IF (blocking_tcp_connections)
	SET (UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS 1) #see upnpconfig.h
ENDIF (blocking_tcp_connections)

OPTION (scriptsupport "script support for IXML document tree, see ixml.h" ON)
SET (IXML_HAVE_SCRIPTSUPPORT ${scriptsupport}) #see upnpconfig.h

OPTION (postwrite "write to the filesystem on otherwise unhandled POST requests" OFF)
SET (UPNP_ENABLE_POST_WRITE ${postwrite}) #see upnpconfig.h

OPTION (reuseaddr "bind the miniserver socket with reuseaddr to allow clean restarts" OFF)
SET (UPNP_MINISERVER_REUSEADDR ${reuseaddr}) #see upnpconfig.h

OPTION (samples "compilation of upnp/sample/ code" ON)

#
# Checks for header files (which aren't needed on Win32)
#
INCLUDE (CheckIncludeFile)

IF (NOT WIN32)
	SET (HAVE_INET_H arpa/inet.h)
	SET (HAVE_FCNTL_H fcntl.h)
	SET (HAVE_INTTYPES_H inttypes.h)
	SET (HAVE_LIMITS_H limits.h)
	SET (HAVE_NETDB_H netdb.h)
	SET (HAVE_IN_H netinet/in.h)
	SET (HAVE_STDLIB_H stdlib.h)
	SET (HAVE_STRING_H string.h)
	SET (HAVE_IOCTL_H sys/ioctl.h)
	SET (HAVE_SOCKET_H sys/socket.h)
	SET (HAVE_TIME_H sys/time.h)
	SET (HAVE_SYSLOG_H syslog.h)
	SET (HAVE_UNISTD_H unistd.h)

	SET (headers
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

	FOREACH (header ${headers})
		CHECK_INCLUDE_FILE (${${header}} ${header})

		IF (NOT ${header})
			MESSAGE (FATAL_ERROR "Header-file ${${header}} not found")
		ENDIF (NOT ${header})
	ENDFOREACH (header ${headers})
ENDIF (NOT WIN32)

#
# Checks for typedefs, structures, and compiler characteristics
#
INCLUDE (TestBigEndian)
TEST_BIG_ENDIAN (big_endian)

CHECK_INCLUDE_FILE (sys/socket.h HAVE_SOCKET_H)
CHECK_INCLUDE_FILE (ws2tcpip.h HAVE_WS2TCPIP_H)

IF (HAVE_SOCKET_H)
	LIST (APPEND CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
ENDIF (HAVE_SOCKET_H)

IF (HAVE_WS2TCPIP_H)
	LIST (APPEND CMAKE_EXTRA_INCLUDE_FILES ws2tcpip.h)
ENDIF (HAVE_WS2TCPIP_H)

INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE (socklen_t SOCKLEN_T)
UNSET (CMAKE_EXTRA_INCLUDE_FILES)

IF (NOT SOCKLEN_T)
	SET (socklen_t "int")
ENDIF (NOT SOCKLEN_T)
#
# Checks for large-file-sensitivity
#
IF (NOT OFF_T_SIZE)
	CHECK_TYPE_SIZE (off_t OFF_T_SIZE)
	SET (UPNP_LARGEFILE_SENSITIVE FALSE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)

	IF (OFF_T_SIZE EQUAL 8)
		MESSAGE (STATUS "System uses 64 bit, no flags needed")
	ELSE (OFF_T_SIZE EQUAL 8)
		UNSET (OFF_T_SIZE CACHE)
		SET (CMAKE_REQUIRED_DEFINITIONS _FILE_OFFSET_BITS=64)
		CHECK_TYPE_SIZE (off_t OFF_T_SIZE)

		IF (OFF_T_SIZE EQUAL 8)
			MESSAGE (STATUS "_FILE_OFFSET_BITS=64 needed")
			SET (UPNP_LARGEFILE_SENSITIVE TRUE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)
			SET (_FILE_OFFSET_BITS 64 CACHE BOOL "Number of bits in a file offset, on hosts where this is settable" FORCE)
		ELSE (OFF_T_SIZE EQUAL 8)
			UNSET (OFF_T_SIZE CACHE)
			SET (CMAKE_REQUIRED_DEFINITIONS _LARGE_FILES)
			CHECK_TYPE_SIZE (off_t OFF_T_SIZE)

			IF (OFF_T_SIZE EQUAL 8)
				MESSAGE (STATUS "_LARGE_FILES needed")
				SET (_LARGE_FILES TRUE CACHE BOOL "Define for large files, on AIX-style hosts." FORCE)
				SET (UPNP_LARGEFILE_SENSITIVE TRUE CACHE BOOL "whether the system defaults to 32bit off_t but can do 64bit when requested" FORCE)
			ENDIF (OFF_T_SIZE EQUAL 8)
		ENDIF (OFF_T_SIZE EQUAL 8)
	ENDIF (OFF_T_SIZE EQUAL 8)
ENDIF (NOT OFF_T_SIZE)

UNSET (CMAKE_REQUIRED_DEFINITIONS)

#
# Checks for library functions
#
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS (fseeko HAVE_FSEEKO)

IF (NOT HAVE_FSEEKO)
	SET (CMAKE_REQUIRED_DEFINITIONS _LARGEFILE_SOURCE)
	CHECK_FUNCTION_EXISTS (fseeko HAVE_FSEEKO)
	UNSET (CMAKE_REQUIRED_DEFINITIONS)

	IF (HAVE_FSEEKO)
		SET (_LARGEFILE_SOURCE TRUE CACHE BOOL "Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2)." FORCE)
	ENDIF (HAVE_FSEEKO)
ENDIF (NOT HAVE_FSEEKO)

CHECK_FUNCTION_EXISTS (strnlen HAVE_STRNLEN)
CHECK_FUNCTION_EXISTS (strndup HAVE_STRNDUP)

IF (Solaris)
	SET (CMAKE_REQUIRED_LIBRARIES socket)
	CHECK_FUNCTION_EXISTS (bind HAVE_SOCKET)
	SET (CMAKE_REQUIRED_LIBRARIES nsl)
	CHECK_FUNCTION_EXISTS (gethostbyname HAVE_NSL])
	SET (CMAKE_REQUIRED_LIBRARIES rt)
	CHECK_FUNCTION_EXISTS (sched_getparam HAVE_RT)
	UNSET (CMAKE_REQUIRED_LIBRARIES)
ENDIF (Solaris)

#
# Checks for POSIX Threads
#
INCLUDE (FindThreads)

#
# Determine if pthread_rwlock_t is available
#
SET (CMAKE_REQUIRED_LIBRARIES Threads::Threads)
SET (CMAKE_EXTRA_INCLUDE_FILES pthread.h)
CHECK_TYPE_SIZE (pthread_rwlock_t UPNP_USE_RWLOCK)
UNSET (CMAKE_REQUIRED_LIBRARIES)
UNSET (CMAKE_EXTRA_INCLUDE_FILES)

IF (open_ssl)
	INCLUDE (FindOpenSSL)

	IF (NOT OPENSSL_FOUND)
		MESSAGE (FATAL_ERROR "openssl not found")
	ENDIF (NOT OPENSSL_FOUND)
ENDIF (open_ssl)


CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/upnp/inc/upnpconfig.h.cm ${CMAKE_CURRENT_BINARY_DIR}/upnp/inc/upnpconfig.h)
CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/upnp/sample/common/config_sample.h.cm ${CMAKE_CURRENT_BINARY_DIR}/upnp/sample/common/config_sample.h)
CONFIGURE_FILE (${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h.cm ${CMAKE_CURRENT_BINARY_DIR}/autoconfig.h)

ADD_SUBDIRECTORY (ixml)
ADD_SUBDIRECTORY (upnp)

ENABLE_TESTING()

FILE (GLOB XML_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/ixml/test/testdata/*.xml
)

ADD_CUSTOM_TARGET (check
	COMMAND ${CMAKE_CTEST_COMMAND}
	DEPENDS ixml_test
		test_init
		test_list
		test_log
		test_url
)

ADD_TEST (NAME IXML_TEST
	COMMAND ixml_test ${XML_FILES}
)

ADD_TEST (NAME UPNP_INIT
	COMMAND test_init
)

ADD_TEST (NAME UPNP_LIST
	COMMAND test_list
)

ADD_TEST (NAME UPNP_LOG
	COMMAND test_log
)

ADD_TEST (NAME UPNP_URL
	COMMAND test_url
)