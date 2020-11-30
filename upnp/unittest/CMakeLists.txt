ADD_EXECUTABLE (test_upnp_unittest
	EXCLUDE_FROM_ALL
	main.c
	templates/FirstObject.c
	templates/templates.c
	templates/SecondObject.c
)

TARGET_LINK_LIBRARIES (test_upnp_unittest
	upnp_shared
)

ADD_EXECUTABLE (test_upnp_unittest_static
	EXCLUDE_FROM_ALL
	main.c
	templates/FirstObject.c
	templates/templates.c
	templates/SecondObject.c
)

TARGET_LINK_LIBRARIES (test_upnp_unittest_static
	upnp_static
)
