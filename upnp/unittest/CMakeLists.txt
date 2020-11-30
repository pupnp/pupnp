ADD_EXECUTABLE (test_upnp_unittest
	main.c
	templates/FirstObject.c
	templates/templates.c
	templates/SecondObject.c
)

TARGET_LINK_LIBRARIES (test_upnp_unittest
	upnp_shared
)

ADD_EXECUTABLE (test_upnp_unittest_static
	main.c
	templates/FirstObject.c
	templates/templates.c
	templates/SecondObject.c
)

TARGET_LINK_LIBRARIES (test_upnp_unittest_static
	upnp_static
)
