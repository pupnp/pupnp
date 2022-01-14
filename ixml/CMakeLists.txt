if (BUILD_TESTING)
	enable_testing()
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

if (UPNP_BUILD_SHARED)
	add_library (ixml_shared SHARED
		${IXML_SOURCES}
	)

	add_library (IXML::Shared ALIAS ixml_shared)

	target_compile_definitions (ixml_shared
		PRIVATE $<$<BOOL:${script_support}>:IXML_HAVE_SCRIPTSUPPORT>
		PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
		PUBLIC $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>
		PUBLIC $<$<BOOL:${MSVC}>:LIBUPNP_EXPORTS>
		PUBLIC $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
		PUBLIC $<$<BOOL:${MSVC}>:_CRT_NONSTDC_NO_DEPRECATE>
		PUBLIC $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_DEPRECATE>
		PUBLIC $<$<BOOL:${_LARGE_FILES}>:_LARGE_FILES>
		PUBLIC $<$<BOOL:${_FILE_OFFSET_BITS}>:_FILE_OFFSET_BITS=64> # A bit hacky, either it is defined to 64 or not defined at all, everything other than 0 is true
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
		VERSION ${IXML_VERSION_STRING}
		SOVERSION ${IXML_VERSION_MAJOR}
		PUBLIC_HEADER "${IXML_HEADERS}"
	)

	install (TARGETS ixml_shared
		EXPORT IXML
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT IXML_Development
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT IXML_Development
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT IXML_RunTime
			NAMELINK_COMPONENT IXML_Development
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT IXML_Development
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			COMPONENT IXML_RunTime
	)
endif()

if (UPNP_BUILD_STATIC)
	add_library (ixml_static STATIC
		${IXML_SOURCES}
	)

	add_library (IXML::Static ALIAS ixml_static)

	set_target_properties (ixml_static PROPERTIES
		OUTPUT_NAME ixml${STATIC_POSTFIX}
		EXPORT_NAME IXML::Static
		VERSION ${IXML_VERSION_MAJOR}
		SOVERSION ${IXML_VERSION_STRING}
		PUBLIC_HEADER "${IXML_HEADERS}"
	)

	target_compile_definitions (ixml_static
		PRIVATE $<$<BOOL:${script_support}>:IXML_HAVE_SCRIPTSUPPORT>
		PUBLIC UPNP_STATIC_LIB
		PUBLIC $<$<BOOL:${MSVC}>:UPNP_USE_MSVCPP>
		PUBLIC $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
		PUBLIC $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
		PUBLIC $<$<BOOL:${MSVC}>:_CRT_NONSTDC_NO_DEPRECATE>
		PUBLIC $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_DEPRECATE>
		PUBLIC $<$<BOOL:${_LARGE_FILES}>:_LARGE_FILES>
		PUBLIC $<$<BOOL:${_FILE_OFFSET_BITS}>:_FILE_OFFSET_BITS=64> # A bit hacky, either it is defined to 64 or not defined at all, everything other than 0 is true
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
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT IXML_Development
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT IXML_Development
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT IXML_RunTime
			NAMELINK_COMPONENT IXML_Development
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/upnp
			COMPONENT IXML_Development
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			COMPONENT IXML_RunTime
	)
endif()

install (EXPORT IXML
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IXML
)
