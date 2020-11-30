# #
# # "Makefile.am" for "libupnp/upnp"
# #
# # Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
# #

ADD_SUBDIRECTORY (test)
ADD_SUBDIRECTORY (unittest)

IF (samples)
	ADD_SUBDIRECTORY (sample)
ENDIF (samples)

SET (UPNP_SOURCES
	src/api/ActionComplete.c
	src/api/ActionRequest.c
	src/api/Discovery.c
	src/api/Event.c
	src/api/EventSubscribe.c
	src/api/ExtraHeaders.c
	src/api/FileInfo.c
	src/api/StateVarComplete.c
	src/api/StateVarRequest.c
	src/api/SubscriptionRequest.c
	src/api/UpnpString.c
	src/api/upnpapi.c
	src/api/upnpdebug.c
	src/genlib/client_table/ClientSubscription.c
	src/genlib/client_table/client_table.c
	src/genlib/miniserver/miniserver.c
	src/genlib/net/sock.c
	src/genlib/net/http/httpparser.c
	src/genlib/net/http/httpreadwrite.c
	src/genlib/net/http/parsetools.c
	src/genlib/net/http/statcodes.c
	src/genlib/net/http/webserver.c
	src/genlib/net/uri/uri.c
	src/genlib/service_table/service_table.c
	src/genlib/util/list.c
	src/genlib/util/membuffer.c
	src/genlib/util/strintmap.c
	src/genlib/util/upnp_timeout.c
	src/genlib/util/util.c
	src/threadutil/FreeList.c
	src/threadutil/LinkedList.c
	src/threadutil/ThreadPool.c
	src/threadutil/TimerThread.c
	src/urlconfig/urlconfig.c
)

IF (gena)
	LIST (APPEND UPNP_SOURCES
		src/gena/gena_device.c
		src/gena/gena_ctrlpt.c
		src/gena/gena_callback2.c
	)
ENDIF (gena)

IF (soap)
	LIST (APPEND UPNP_SOURCES
		src/soap/soap_device.c
		src/soap/soap_ctrlpt.c
		src/soap/soap_common.c
	)
ENDIF (soap)

IF (ssdp)
	LIST (APPEND UPNP_SOURCES
		src/ssdp/ssdp_ResultData.c
		src/ssdp/ssdp_device.c
		src/ssdp/ssdp_ctrlpt.c
		src/ssdp/ssdp_server.c
	)
ENDIF (ssdp)

IF (tools)
	LIST (APPEND UPNP_SOURCES
		src/api/upnptools.c
	)
ENDIF (tools)

IF (uuid)
	LIST (APPEND UPNP_SOURCES
		src/uuid/md5.c
		src/uuid/sysdep.c
		src/uuid/uuid.c
	)
ENDIF (uuid)

ADD_LIBRARY (upnp_shared SHARED
	${UPNP_SOURCES}
)

ADD_LIBRARY (UPNP::SHARED ALIAS upnp_shared)

TARGET_COMPILE_DEFINITIONS (upnp_shared
	PRIVATE $<$<CONFIG:Debug>:STATS>
	PRIVATE $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
)

TARGET_INCLUDE_DIRECTORIES (upnp_shared
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
	PRIVATE $<TARGET_PROPERTY:ixml_static,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/$<CONFIG>>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/$<CONFIG>/upnp/inc/>
)

LIST (APPEND UPNP_HEADERS
	inc/ActionComplete.h
	inc/ActionRequest.h
	inc/Callback.h
	inc/Discovery.h
	inc/Event.h
	inc/EventSubscribe.h
	inc/ExtraHeaders.h
	inc/FileInfo.h
	inc/ithread.h
	inc/list.h
	inc/StateVarComplete.h
	inc/StateVarRequest.h
	inc/SubscriptionRequest.h
	inc/TemplateInclude.h
	inc/TemplateSource.h
	inc/TemplateUndef.h
	inc/UpnpString.h
	inc/upnp.h
	inc/UpnpGlobal.h
	inc/UpnpInet.h
	inc/UpnpIntTypes.h
	inc/UpnpStdInt.h
	inc/UpnpUniStd.h
	$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/inc/upnpdebug.h>
	${UPNP_BINARY_DIR}/Release/upnp/inc/upnpconfig.h
)

IF (ENABLE_TOOLS)
	LIST (APPEND UPNP_HEADERS
		inc/upnptools.h
	)
ENDIF (ENABLE_TOOLS)

SET_TARGET_PROPERTIES (upnp_shared PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	OUTPUT_NAME upnp
	EXPORT_NAME UPNP::SHARED
	VERSION ${UPNP_VERSION}
	SOVERSION ${UPNP_VERSION_MAJOR}
	PUBLIC_HEADER "${UPNP_HEADERS}"
)

TARGET_COMPILE_DEFINITIONS (upnp_shared
	PRIVATE $<$<CONFIG:Debug>:STATS>
	PRIVATE $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
	PRIVATE $<TARGET_PROPERTY:ixml_shared,INTERFACE_COMPILE_DEFINITIONS>
)

TARGET_INCLUDE_DIRECTORIES (upnp_shared
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
#OTHER	PRIVATE $<TARGET_PROPERTY:ixml_static,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/$<CONFIG>>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/$<CONFIG>/upnp/inc/>
	PRIVATE $<TARGET_PROPERTY:Threads::Threads,INTERFACE_INCLUDE_DIRECTORIES>
)

IF (WIN32)
	TARGET_LINK_LIBRARIES (upnp_shared
		PRIVATE ws2_32
		PRIVATE iphlpapi
	)
ENDIF (WIN32)

TARGET_LINK_LIBRARIES (upnp_shared
	PUBLIC ixml_shared
	PUBLIC Threads::Threads
)

IF (UPNP_ENABLE_OPEN_SSL)
	TARGET_LINK_LIBRARIES (upnp_shared
		PRIVATE OpenSSL::SSL
	)
ENDIF (UPNP_ENABLE_OPEN_SSL)

INSTALL (TARGETS upnp_shared
	EXPORT UPNPCONFIG
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
)

ADD_LIBRARY (upnp_static STATIC
	${UPNP_SOURCES}
)

ADD_LIBRARY (UPNP::STATIC ALIAS upnp_static)

TARGET_COMPILE_DEFINITIONS (upnp_static
	PRIVATE UPNP_STATIC_LIB
	PRIVATE $<$<CONFIG:Debug>:STATS>
	PRIVATE $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
	PRIVATE $<TARGET_PROPERTY:ixml_static,INTERFACE_COMPILE_DEFINITIONS>
)

TARGET_INCLUDE_DIRECTORIES (upnp_static
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
	PRIVATE $<TARGET_PROPERTY:ixml_static,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/$<CONFIG>>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/$<CONFIG>/upnp/inc/>
	PRIVATE $<TARGET_PROPERTY:Threads::Threads,INTERFACE_INCLUDE_DIRECTORIES>
)

SET_TARGET_PROPERTIES(upnp_static PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	OUTPUT_NAME upnp
	EXPORT_NAME UPNP::STATIC
)

#IF (WIN32)
#	TARGET_LINK_LIBRARIES (upnp_static
#		PUBLIC ixml_static
#		PRIVATE Threads::Static
#	)
#ELSE (WIN32)
	TARGET_LINK_LIBRARIES (upnp_static
		INTERFACE ixml_static
		INTERFACE Threads::Static
	)
#ENDIF (WIN32)

IF (UPNP_ENABLE_OPEN_SSL)
	TARGET_LINK_LIBRARIES (upnp_static
		PRIVATE OpenSSL::SSL
	)
ENDIF (UPNP_ENABLE_OPEN_SSL)

INSTALL (TARGETS upnp_static
	EXPORT UPNPCONFIG
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp

INSTALL (EXPORT UPNPCONFIG
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

# # inet_pton (needed on Win32, compiles to nothing elsewhere)
# libupnp_la_SOURCES += \
# 	src/inet_pton.c \
# 	src/inc/inet_pton.h
# 
# EXTRA_DIST = \
# 	m4/libupnp.m4 \
# 	src/win_dll.c
# 
# CLEANFILES = libupnp_err.log libupnp_info.log
