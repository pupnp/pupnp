ADD_EXECUTABLE (unittest
	main.c
	templates/FirstObject.c
	templates/templates.c
	templates/SecondObject.c
)

TARGET_LINK_LIBRARIES (unittest
	upnp_shared
)

# unittest_SOURCES = \
# 	templates/FirstObject.h \
# 	templates/templates.h \
# 	templates/SecondObject.h \
# 	templates/TheStruct.h
