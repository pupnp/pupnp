include (GoogleTest)

addGTest(test_template test_template.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

addGTest(test_UpnpHttpHeaderList test_UpnpHttpHeaderList.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

if (NOT WIN32)
	addGTest (test_upnpapi test_upnpapi.cpp
		ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
	)
endif()
