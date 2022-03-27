# Only set library prefix with MSVC toolchain
if (MSVC)
	set (WIN_PREFIX "lib")
endif()

if (UPNP_BUILD_SAMPLES)
	add_subdirectory (sample)
endif()

set (UPNP_SOURCES
	src/api/upnpapi.c
	src/api/upnpdebug.c
	src/api/UpnpActionComplete.c
	src/api/UpnpActionRequest.c
	src/api/UpnpDiscovery.c
	src/api/UpnpEvent.c
	src/api/UpnpEventSubscribe.c
	src/api/UpnpExtraHeaders.c
	src/api/UpnpFileInfo.c
	src/api/UpnpStateVarComplete.c
	src/api/UpnpStateVarRequest.c
	src/api/UpnpString.c
	src/api/UpnpSubscriptionRequest.c
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

if (UPNP_ENABLE_GENA)
	list (APPEND UPNP_SOURCES
		src/gena/gena_device.c
		src/gena/gena_ctrlpt.c
		src/gena/gena_callback2.c
	)
endif()

if (UPNP_ENABLE_SOAP)
	list (APPEND UPNP_SOURCES
		src/soap/soap_device.c
		src/soap/soap_ctrlpt.c
		src/soap/soap_common.c
	)
endif()

if (UPNP_ENABLE_SSDP)
	list (APPEND UPNP_SOURCES
		src/ssdp/SSDPResultData.c
		src/ssdp/SSDPResultDataCallback.c
		src/ssdp/ssdp_device.c
		src/ssdp/ssdp_ctrlpt.c
		src/ssdp/ssdp_server.c
	)
endif()

if (UPNP_ENABLE_GENA OR UPNP_ENABLE_OPTIONAL_SSDP_HEADERS)
	list (APPEND UPNP_SOURCES
		src/uuid/md5.c
		src/uuid/sysdep.c
		src/uuid/uuid.c
	)
endif()

list (APPEND UPNP_HEADERS
	inc/Callback.h
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
	${PUPNP_BINARY_DIR}/upnp/inc/upnpconfig.h
)

if (UPNP_ENABLE_HELPER_API_TOOLS)
	list (APPEND UPNP_SOURCES
		src/api/upnptools.c
	)

	list (APPEND UPNP_HEADERS
		inc/upnptools.h
	)
endif()

if (UPNP_BUILD_SHARED)
	add_library (upnp_shared SHARED
		${UPNP_SOURCES}
	)

	add_library (UPNP::Shared ALIAS upnp_shared)

	set_target_properties (upnp_shared PROPERTIES
		C_VISIBILITY_PRESET "hidden"
		EXPORT_NAME UPNP::Shared
		OUTPUT_NAME ${WIN_PREFIX}upnp
		PUBLIC_HEADER "${UPNP_HEADERS}"
		SOVERSION ${UPNP_VERSION_MAJOR}
		VERSION ${UPNP_VERSION_STRING}
	)

	target_compile_definitions (upnp_shared
		PRIVATE $<$<CONFIG:Debug>:STATS>
		PRIVATE LIBUPNP_EXPORTS
		PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
	)

	target_compile_options (upnp_shared
		PRIVATE $<$<BOOL:${HAVE_MACRO_PREFIX_MAP}>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.>
	)

	target_include_directories (upnp_shared
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
		PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
		PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
		PUBLIC $<BUILD_INTERFACE:${PUPNP_BINARY_DIR}/>
		PUBLIC $<BUILD_INTERFACE:${PUPNP_BINARY_DIR}/upnp/inc/>
	)

	target_link_libraries (upnp_shared
		PRIVATE $<$<BOOL:${UPNP_ENABLE_OPEN_SSL}>:OpenSSL::SSL>
		PRIVATE $<$<PLATFORM_ID:Windows>:ws2_32>
		PRIVATE $<$<PLATFORM_ID:Windows>:iphlpapi>
		PUBLIC ixml_shared
		PUBLIC Threads::Shared
	)

	install (TARGETS upnp_shared
		EXPORT UPNP
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT UPNP_Development
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT UPNP_Development
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT UPNP_RunTime
			NAMELINK_COMPONENT UPNP_Development
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT UPNP_Development
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			COMPONENT UPNP_RunTime
	)
endif()

if (UPNP_BUILD_STATIC)
	add_library (upnp_static STATIC
		${UPNP_SOURCES}
	)

	add_library (UPNP::Static ALIAS upnp_static)

	set_target_properties (upnp_static PROPERTIES
		C_VISIBILITY_PRESET "hidden"
		EXPORT_NAME UPNP::Static
		OUTPUT_NAME ${WIN_PREFIX}upnp${STATIC_POSTFIX}
		PUBLIC_HEADER "${UPNP_HEADERS}"
		SOVERSION ${UPNP_VERSION_STRING}
		VERSION ${UPNP_VERSION_MAJOR}
	)

	target_compile_definitions (upnp_static
		PRIVATE UPNP_STATIC_LIB
		PRIVATE LIBUPNP_EXPORTS
		PRIVATE $<$<CONFIG:Debug>:STATS>
		PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
	)

	target_compile_options (upnp_static
		PRIVATE $<$<BOOL:${HAVE_MACRO_PREFIX_MAP}>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.>
	)

	target_include_directories (upnp_static
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/threadutil/
		PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
		PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
		PUBLIC $<BUILD_INTERFACE:${PUPNP_BINARY_DIR}/>
		PUBLIC $<BUILD_INTERFACE:${PUPNP_BINARY_DIR}/upnp/inc/>
	)

	target_link_libraries (upnp_static
		PRIVATE $<$<BOOL:${UPNP_ENABLE_OPEN_SSL}>:OpenSSL::SSL>
		PRIVATE $<$<PLATFORM_ID:Windows>:ws2_32>
		PRIVATE $<$<PLATFORM_ID:Windows>:iphlpapi>
		PUBLIC ixml_static
		PUBLIC Threads::Static
	)

	install (TARGETS upnp_static
		EXPORT UPNP
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT UPNP_Development
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT UPNP_Development
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT UPNP_RunTime
			NAMELINK_COMPONENT UPNP_Development
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT UPNP_Development
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			COMPONENT UPNP_RunTime
	)
endif()

install (EXPORT UPNP
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UPNP
)

if (BUILD_TESTING)
	enable_testing()
	add_subdirectory (test)
endif()
