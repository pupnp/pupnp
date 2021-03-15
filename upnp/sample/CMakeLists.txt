IF (client)
	ADD_EXECUTABLE (tv_ctrlpt
		common/sample_util.c
		common/tv_ctrlpt.c
		linux/tv_ctrlpt_main.c
	)

	TARGET_INCLUDE_DIRECTORIES (tv_ctrlpt
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/common
	)

	TARGET_LINK_LIBRARIES (tv_ctrlpt
		upnp_shared
	)

	INSTALL (TARGETS tv_ctrlpt
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
ENDIF (client)

IF (client AND device)
	ADD_EXECUTABLE (tv_combo
		common/sample_util.c
		common/tv_ctrlpt.c
		common/tv_device.c
		linux/tv_combo_main.c
	)

	TARGET_INCLUDE_DIRECTORIES (tv_combo
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/common
	)

	TARGET_LINK_LIBRARIES (tv_combo
		upnp_shared
	)

	INSTALL (TARGETS tv_combo
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
ENDIF (client AND device)

IF (device)
	ADD_EXECUTABLE (tv_device
		common/sample_util.c
		common/tv_device.c
		linux/tv_device_main.c
	)

	TARGET_INCLUDE_DIRECTORIES (tv_device
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/common
	)

	TARGET_LINK_LIBRARIES (tv_device
		upnp_shared
	)

	INSTALL (TARGETS tv_device
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

	INSTALL (DIRECTORY web/
		DESTINATION ${CMAKE_INSTALL_DATADIR}/upnp
	)
ENDIF (device)
