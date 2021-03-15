set_source_files_properties (common/sample_util.c
	common/tv_ctrlpt.c
	common/tv_device.c
	linux/tv_combo_main.c
	linux/tv_ctrlpt_main.c
	linux/tv_device_main.c
	PROPERTIES LANGUAGE CXX
)

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
		upnp_shared
	)

	install (TARGETS tv_ctrlpt
		DESTINATION ${CMAKE_INSTALL_BINDIR}
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
		upnp_shared
	)

	install (TARGETS tv_combo
		DESTINATION ${CMAKE_INSTALL_BINDIR}
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
		upnp_shared
	)

	install (TARGETS tv_device
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

	install (DIRECTORY web/
		DESTINATION ${CMAKE_INSTALL_DATADIR}/upnp
	)
endif()
