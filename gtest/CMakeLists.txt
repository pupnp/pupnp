include (GoogleTest)

addGTest (test-upnp-upnpapi test_upnpapi.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

addGTest(test_UpnpHttpHeaderList test_UpnpHttpHeaderList.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)
