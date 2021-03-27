include (GoogleTest)

addGMock (test-upnp-upnpapi test_upnpapi.cpp
	ADDITIONAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../upnp/src/threadutil/
)

set_tests_properties (test-upnp-UpnpApiIPv4TestSuite.UpnpGetIfInfo_called_with_valid_interface
	PROPERTIES WILL_FAIL TRUE
)

set_tests_properties (test-upnp-UpnpApiIPv4TestSuite.UpnpGetIfInfo_called_with_unknown_interface
	PROPERTIES WILL_FAIL TRUE
)

set_tests_properties (test-upnp-UpnpApiIPv4TestSuite.initialize_default_UpnpInit2
	PROPERTIES WILL_FAIL TRUE
)

set_tests_properties (test-upnp-UpnpApiIPv4TestSuite.UpnpGetIfInfo_called_with_valid_interface-static
	PROPERTIES WILL_FAIL TRUE
)

set_tests_properties (test-upnp-UpnpApiIPv4TestSuite.UpnpGetIfInfo_called_with_unknown_interface-static
	PROPERTIES WILL_FAIL TRUE
)
