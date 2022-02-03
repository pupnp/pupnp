if (UPNP_BUILD_SHARED)
	set (UPNP_LIBRARY upnp_shared)
else ()
	set (UPNP_LIBRARY upnp_static)
endif()

if (client)
	add_executable (tv_ctrlpt
		common/sample_util.c
		common/tv_ctrlpt.c
		linux/tv_ctrlpt_main.c
	)

	target_include_directories (tv_ctrlpt
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/common
	)

	target_link_libraries (tv_ctrlpt
		${UPNP_LIBRARY}
	)

	install (TARGETS tv_ctrlpt
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		COMPONENT UPNP_samples
	)
endif()

if (client AND device)
	add_executable (tv_combo
		common/sample_util.c
		common/tv_ctrlpt.c
		common/tv_device.c
		linux/tv_combo_main.c
	)

	target_include_directories (tv_combo
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/common
	)

	target_link_libraries (tv_combo
		${UPNP_LIBRARY}
	)

	install (TARGETS tv_combo
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		COMPONENT UPNP_samples
	)
endif()

if (device)
	add_executable (tv_device
		common/sample_util.c
		common/tv_device.c
		linux/tv_device_main.c
	)

	target_include_directories (tv_device
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/common
	)

	target_link_libraries (tv_device
		${UPNP_LIBRARY}
	)

	install (TARGETS tv_device
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		COMPONENT UPNP_samples
	)

	install (DIRECTORY web/
		DESTINATION ${CMAKE_INSTALL_DATADIR}/upnp
		COMPONENT UPNP_samples
	)
endif()
