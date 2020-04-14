
# # check / distcheck tests
# check_PROGRAMS = test_init test_url test_log test_list
# TESTS = test_init test_url test_log test_list
# test_init_SOURCES = test/test_init.c
# test_url_SOURCES = test/test_url.c
# test_log_SOURCES = test/test_log.c
# test_list_SOURCES = test/test_list.c

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
