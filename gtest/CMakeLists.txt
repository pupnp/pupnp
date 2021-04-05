include (GoogleTest)

	addGTest (test_workflow test_workflow.cpp
		ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
