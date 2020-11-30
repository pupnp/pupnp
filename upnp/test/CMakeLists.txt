ADD_EXECUTABLE (test_upnp_init
	EXCLUDE_FROM_ALL
	test_init.c
)

TARGET_LINK_LIBRARIES (test_upnp_init
	upnp_shared
)

ADD_EXECUTABLE (test_upnp_init_static
	EXCLUDE_FROM_ALL
	test_init.c
)

TARGET_LINK_LIBRARIES (test_upnp_init_static
	upnp_static
)

ADD_EXECUTABLE (test_upnp_list
	EXCLUDE_FROM_ALL
	test_list.c
)

TARGET_LINK_LIBRARIES (test_upnp_list
	upnp_shared
)

ADD_EXECUTABLE (test_upnp_list_static
	EXCLUDE_FROM_ALL
	test_list.c
)

TARGET_LINK_LIBRARIES (test_upnp_list_static
	upnp_static
)

ADD_EXECUTABLE (test_upnp_log
	EXCLUDE_FROM_ALL
	test_log.c
)

TARGET_LINK_LIBRARIES (test_upnp_log
	upnp_shared
)

ADD_EXECUTABLE (test_upnp_log_static
	EXCLUDE_FROM_ALL
	test_log.c
)

TARGET_LINK_LIBRARIES (test_upnp_log_static
	upnp_static
)

ADD_EXECUTABLE (test_upnp_url
	EXCLUDE_FROM_ALL
	test_url.c
)

TARGET_LINK_LIBRARIES (test_upnp_url
	upnp_shared
)

ADD_EXECUTABLE (test_upnp_url_static
	EXCLUDE_FROM_ALL
	test_url.c
)

TARGET_LINK_LIBRARIES (test_upnp_url_static
	upnp_static
)
