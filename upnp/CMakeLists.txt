# #
# # "Makefile.am" for "libupnp/upnp"
# #
# # Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
# #

ADD_SUBDIRECTORY (test)

ADD_LIBRARY (upnp_objects OBJECT
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

TARGET_INCLUDE_DIRECTORIES (upnp_objects
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/inc/
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/inc/
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
	PRIVATE ${UPNP_BINARY_DIR}
)

TARGET_LINK_LIBRARIES (upnp_objects
	PUBLIC ixml_objects
)

IF (debug)
	SET_PROPERTY (TARGET upnp_objects
		APPEND PROPERTY SOURCES
			src/api/upnpdebug.c 
	)

	TARGET_COMPILE_DEFINITIONS (upnp_objects
		PRIVATE STATS
	)
ENDIF (debug)

IF (gena)
	SET_PROPERTY (TARGET upnp_objects
		APPEND PROPERTY SOURCES
			src/gena/gena_device.c
			src/gena/gena_ctrlpt.c
			src/gena/gena_callback2.c
	)
ENDIF (gena)

IF (soap)
	SET_PROPERTY (TARGET upnp_objects
		APPEND PROPERTY SOURCES
			src/soap/soap_device.c
			src/soap/soap_ctrlpt.c
			src/soap/soap_common.c
	)
ENDIF (soap)

IF (ssdp)
	SET_PROPERTY (TARGET upnp_objects
		APPEND PROPERTY SOURCES
			src/ssdp/ssdp_ResultData.c
			src/ssdp/ssdp_ResultData.h
			src/ssdp/ssdp_device.c
			src/ssdp/ssdp_ctrlpt.c
			src/ssdp/ssdp_server.c
	)
ENDIF (ssdp)

IF (tools)
	SET_PROPERTY (TARGET upnp_objects
		APPEND PROPERTY SOURCES
			src/api/upnptools.c
	)
ENDIF (tools)

IF (uuid)
	SET_PROPERTY (TARGET upnp_objects
		APPEND PROPERTY SOURCES
			src/uuid/md5.c
			src/uuid/sysdep.c
			src/uuid/uuid.c
	)
ENDIF (uuid)

SET_PROPERTY (TARGET upnp_objects
	PROPERTY POSITION_INDEPENDENT_CODE 1
)

ADD_LIBRARY (upnp_shared SHARED
	$<TARGET_OBJECTS:upnp_objects>
)

SET_TARGET_PROPERTIES(upnp_shared PROPERTIES
	OUTPUT_NAME upnp
	VERSION ${UPNP_VERSION}
	SOVERSION ${UPNP_VERSION_MAJOR}
)

TARGET_LINK_LIBRARIES (upnp_shared
	INTERFACE ixml_shared
	INTERFACE upnp_objects
)

IF (UPNP_ENABLE_OPEN_SSL)
	TARGET_LINK_LIBRARIES (upnp_shared
		PRIVATE OpenSSL::SSL
	)
ENDIF (UPNP_ENABLE_OPEN_SSL)

ADD_LIBRARY (upnp_static STATIC
	$<TARGET_OBJECTS:upnp_objects>
)

SET_TARGET_PROPERTIES(upnp_static PROPERTIES
	OUTPUT_NAME upnp
)

TARGET_LINK_LIBRARIES (upnp_static
	INTERFACE ixml_static
	INTERFACE upnp_objects
)


IF (UPNP_ENABLE_OPEN_SSL)
	TARGET_LINK_LIBRARIES (upnp_static
		PRIVATE OpenSSL::SSL
	)
ENDIF (UPNP_ENABLE_OPEN_SSL)
# 
# 
# SUBDIRS = doc . sample unittest
# 
# upnpincludedir = $(includedir)/upnp
# upnpinclude_HEADERS = \
# 	inc/ActionComplete.h \
# 	inc/ActionRequest.h \
# 	inc/Callback.h \
# 	inc/Discovery.h \
# 	inc/Event.h \
# 	inc/EventSubscribe.h \
# 	inc/ExtraHeaders.h \
# 	inc/FileInfo.h \
# 	inc/list.h \
# 	inc/StateVarComplete.h \
# 	inc/StateVarRequest.h \
# 	inc/SubscriptionRequest.h \
# 	inc/TemplateInclude.h \
# 	inc/TemplateSource.h \
# 	inc/TemplateUndef.h \
# 	inc/UpnpString.h \
# 	inc/upnp.h \
# 	inc/upnpdebug.h \
# 	inc/UpnpGlobal.h \
# 	inc/UpnpInet.h \
# 	inc/UpnpIntTypes.h \
# 	inc/UpnpStdInt.h \
# 	inc/UpnpUniStd.h
# 
# nodist_upnpinclude_HEADERS = inc/upnpconfig.h 
# if ENABLE_TOOLS
# upnpinclude_HEADERS += inc/upnptools.h 
# endif

# libupnp_la_SOURCES = \
# 	src/inc/config.h \
# 	src/inc/client_table.h \
# 	src/inc/ClientSubscription.h \
# 	src/inc/gena.h \
# 	src/inc/gena_ctrlpt.h \
# 	src/inc/gena_device.h \
# 	src/inc/httpparser.h \
# 	src/inc/httpreadwrite.h \
# 	src/inc/md5.h \
# 	src/inc/membuffer.h \
# 	src/inc/miniserver.h \
# 	src/inc/netall.h \
# 	src/inc/parsetools.h \
# 	src/inc/service_table.h \
# 	src/inc/soaplib.h \
# 	src/inc/sock.h \
# 	src/inc/statcodes.h \
# 	src/inc/statuscodes.h \
# 	src/inc/strintmap.h \
# 	src/inc/ssdplib.h \
# 	src/inc/sysdep.h \
# 	src/inc/unixutil.h \
# 	src/inc/upnpapi.h \
# 	src/inc/upnp_timeout.h \
# 	src/inc/uri.h \
# 	src/inc/urlconfig.h \
# 	src/inc/upnputil.h \
# 	src/inc/uuid.h \
# 	src/inc/VirtualDir.h \
# 	src/inc/webserver.h
# 
# # threadutil
# upnpinclude_HEADERS += \
# 	inc/ithread.h
# 
# libupnp_la_SOURCES += \
# 	src/threadutil/FreeList.h \
# 	src/threadutil/LinkedList.h \
# 	src/threadutil/ThreadPool.h \
# 	src/threadutil/TimerThread.h \

# # ssdp
# if ENABLE_SSDP
# libupnp_la_SOURCES += \
# 	src/ssdp/ssdp_ResultData.h \
# endif
# 
# 
# 
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
