if (EXISTS ${BUILD_DIR}/upnp/test/test_init.log)
	file (READ ${BUILD_DIR}/upnp/test/test_init.log logdata)
	message (${logdata})
endif()
