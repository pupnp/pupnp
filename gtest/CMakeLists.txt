include (GoogleTest)

addGTest(test_UpnpHttpHeaderList test_UpnpHttpHeaderList.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)