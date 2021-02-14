if (BUILD_TESTING)
	add_subdirectory (test)
endif()

set (IXML_SOURCES
	src/attr.c
	src/document.c
	src/element.c
	src/ixml.c
	src/ixmldebug.c
	src/ixmlmembuf.c
	src/ixmlparser.c
	src/namedNodeMap.c
	src/node.c
	src/nodeList.c
)

add_library (ixml_shared SHARED
	${IXML_SOURCES}
)

add_library (IXML::Shared ALIAS ixml_shared)

target_compile_definitions (ixml_shared
	PRIVATE $<$<BOOL:${script_support}>:IXML_HAVE_SCRIPTSUPPORT>
	PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
	PUBLIC $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>
	PUBLIC $<$<BOOL:${MSVC}>:LIBUPNP_EXPORTS>
)

target_include_directories (ixml_shared
	PRIVATE ${PUPNP_BINARY_DIR}/
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${PUPNP_SOURCE_DIR}/upnp/inc/>
)

list (APPEND IXML_HEADERS
	inc/ixml.h
	inc/ixmldebug.h
)

set_target_properties (ixml_shared PROPERTIES
	OUTPUT_NAME ixml
	EXPORT_NAME IXML::Shared
	VERSION ${IXML_VERSION}
	SOVERSION ${IXML_VERSION_MAJOR}
	PUBLIC_HEADER "${IXML_HEADERS}"
)

install (TARGETS ixml_shared
	EXPORT IXML
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
)

add_library (ixml_static STATIC
	${IXML_SOURCES}
)

add_library (IXML::Static ALIAS ixml_static)

set_target_properties (ixml_static PROPERTIES
	OUTPUT_NAME ixml${STATIC_POSTFIX}
	EXPORT_NAME IXML::Static
)

target_compile_definitions (ixml_static
	PRIVATE $<$<BOOL:${script_support}>:IXML_HAVE_SCRIPTSUPPORT>
	PUBLIC UPNP_STATIC_LIB
	PUBLIC $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>
	PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
)

if (script_support)
	target_compile_definitions (ixml_static
		PRIVATE IXML_HAVE_SCRIPTSUPPORT
	)
endif()

target_include_directories (ixml_static
	PRIVATE ${PUPNP_BINARY_DIR}/
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/inc/>
	PUBLIC $<BUILD_INTERFACE:${PUPNP_SOURCE_DIR}/upnp/inc/>
)

install (TARGETS ixml_static
	EXPORT IXML
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
)

install (EXPORT IXML
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IXML
)
