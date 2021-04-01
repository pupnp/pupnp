include (GoogleTest)

addGTest (test-upnp-upnpapi test_upnpapi.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)
