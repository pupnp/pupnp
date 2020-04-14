ADD_EXECUTABLE (test_init
	EXCLUDE_FROM_ALL
	test_init.c
)

TARGET_LINK_LIBRARIES (test_init
	upnp_shared
)

ADD_EXECUTABLE (test_list
	EXCLUDE_FROM_ALL
	test_list.c
)

TARGET_LINK_LIBRARIES (test_list
	upnp_shared
)

ADD_EXECUTABLE (test_log
	EXCLUDE_FROM_ALL
	test_log.c
)

TARGET_LINK_LIBRARIES (test_log
	upnp_shared
)

ADD_EXECUTABLE (test_url
	EXCLUDE_FROM_ALL
	test_url.c
)

TARGET_LINK_LIBRARIES (test_url
	upnp_shared
)
