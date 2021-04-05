include (GoogleTest)

	addGTest (test_template test_template.cpp
		ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
