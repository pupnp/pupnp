IF (BUILD_TESTING)
	ADD_SUBDIRECTORY (test)
ENDIF (BUILD_TESTING)

IF (samples)
	ADD_SUBDIRECTORY (sample)
ENDIF (samples)

SET (UPNP_SOURCES
	src/api/UpnpActionComplete.c
	src/api/UpnpActionRequest.c
	src/api/UpnpDiscovery.c
	src/api/UpnpEvent.c
	src/api/UpnpEventSubscribe.c
	src/api/UpnpExtraHeaders.c
	src/api/UpnpFileInfo.c
	src/api/UpnpStateVarComplete.c
	src/api/UpnpStateVarRequest.c
	src/api/UpnpSubscriptionRequest.c
	src/api/UpnpString.c
	src/api/upnpapi.c
	src/api/upnpdebug.c
	src/genlib/client_table/GenlibClientSubscription.c
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
		src/ssdp/SSDPResultData.c
		src/ssdp/SSDPResultDataCallback.c
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

SET_TARGET_PROPERTIES (upnp_shared PROPERTIES
	WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)

TARGET_COMPILE_DEFINITIONS (upnp_shared
	PRIVATE $<$<CONFIG:Debug>:STATS>
	PUBLIC $<IF:$<CONFIG:Debug>,DEBUG UPNP_HAVE_DEBUG,NDEBUG>
)

TARGET_INCLUDE_DIRECTORIES (upnp_shared
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/upnp/inc/>
)

LIST (APPEND UPNP_HEADERS
	inc/Callback.h
	inc/ithread.h
	inc/list.h
	inc/upnp.h
	inc/UpnpActionComplete.h
	inc/UpnpActionRequest.h
	inc/UpnpDiscovery.h
	inc/UpnpEvent.h
	inc/UpnpEventSubscribe.h
	inc/UpnpExtraHeaders.h
	inc/UpnpFileInfo.h
	inc/UpnpGlobal.h
	inc/UpnpInet.h
	inc/UpnpIntTypes.h
	inc/UpnpStateVarComplete.h
	inc/UpnpStateVarRequest.h
	inc/UpnpStdInt.h
	inc/UpnpString.h
	inc/UpnpSubscriptionRequest.h
	inc/UpnpUniStd.h
	${CMAKE_CURRENT_SOURCE_DIR}/inc/upnpdebug.h
	${UPNP_BINARY_DIR}/upnp/inc/upnpconfig.h
)

IF (ENABLE_TOOLS)
	LIST (APPEND UPNP_HEADERS
		inc/upnptools.h
	)
ENDIF (ENABLE_TOOLS)

SET_TARGET_PROPERTIES (upnp_shared PROPERTIES
	OUTPUT_NAME upnp
	EXPORT_NAME UPNP::SHARED
	VERSION ${UPNP_VERSION}
	SOVERSION ${UPNP_VERSION_MAJOR}
	PUBLIC_HEADER "${UPNP_HEADERS}"
)

TARGET_COMPILE_DEFINITIONS (upnp_shared
	PRIVATE $<$<CONFIG:Debug>:STATS>
	PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
)

TARGET_INCLUDE_DIRECTORIES (upnp_shared
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/upnp/inc/>
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
	PUBLIC $<IF:$<CONFIG:Debug>,DEBUG UPNP_HAVE_DEBUG,NDEBUG>
)

TARGET_INCLUDE_DIRECTORIES (upnp_static
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/>
	PUBLIC $<BUILD_INTERFACE:${UPNP_BINARY_DIR}/upnp/inc/>
)

IF (WIN32)
	TARGET_LINK_LIBRARIES (upnp_static
		INTERFACE ws2_32
		INTERFACE iphlpapi
	)
ENDIF (WIN32)

SET_TARGET_PROPERTIES(upnp_static PROPERTIES
	OUTPUT_NAME upnps
	EXPORT_NAME UPNP::STATIC
)

TARGET_LINK_LIBRARIES (upnp_static
	PUBLIC ixml_static
	PUBLIC Threads::Static
)

IF (UPNP_ENABLE_OPEN_SSL)
	TARGET_LINK_LIBRARIES (upnp_static
		PRIVATE OpenSSL::SSL
	)
ENDIF (UPNP_ENABLE_OPEN_SSL)

INSTALL (TARGETS upnp_static
	EXPORT UPNPCONFIG
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
)

INSTALL (EXPORT UPNPCONFIG
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
