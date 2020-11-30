IF (client)
	ADD_EXECUTABLE (tv_ctrlpt
		EXCLUDE_FROM_ALL
		common/sample_util.c
		common/tv_ctrlpt.c
		linux/tv_ctrlpt_main.c
	)

	TARGET_INCLUDE_DIRECTORIES (tv_ctrlpt
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${UPNP_BINARY_DIR}/$<CONFIG>/upnp/sample/common
	)

	TARGET_LINK_LIBRARIES (tv_ctrlpt
		upnp_shared
	)
ENDIF (client)

IF (client AND device)
	ADD_EXECUTABLE (tv_combo
		EXCLUDE_FROM_ALL
		common/sample_util.c
		common/tv_ctrlpt.c
		common/tv_device.c
		linux/tv_combo_main.c
	)

	TARGET_INCLUDE_DIRECTORIES (tv_combo
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${UPNP_BINARY_DIR}/$<CONFIG>/upnp/sample/common
	)

	TARGET_LINK_LIBRARIES (tv_combo
		upnp_shared
	)
ENDIF (client AND device)

IF (device)
	ADD_EXECUTABLE (tv_device
		EXCLUDE_FROM_ALL
		common/sample_util.c
		common/tv_device.c
		linux/tv_device_main.c
	)

	TARGET_INCLUDE_DIRECTORIES (tv_device
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common
		PRIVATE ${UPNP_BINARY_DIR}/$<CONFIG>/upnp/sample/common
	)

	TARGET_LINK_LIBRARIES (tv_device
		upnp_shared
	)
ENDIF (device)