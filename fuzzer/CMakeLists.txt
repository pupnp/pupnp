cmake_minimum_required(VERSION 3.11)

project(fuzzer LANGUAGES C)
add_executable(FuzzIxml FuzzIxml.c)
add_executable(FuzzHttp FuzzHttp.c)

if(NOT DEFINED LOCAL_RUN)
	message("LOCAL_RUN is not defined")

	# GitHub Actions needs this like that
	target_link_libraries(FuzzIxml ixml_static ${LIB_FUZZING_ENGINE})
	target_include_directories(FuzzHttp PRIVATE ${CMAKE_SOURCE_DIR}/upnp/src/threadutil)
	target_link_libraries(FuzzHttp upnp_static ${LIB_FUZZING_ENGINE})
else()
	message("LOCAL_RUN is ${LOCAL_RUN}")

	set(MY_CUSTOM_PATH ..)
	find_library(LIB_IXML ixml REQUIRED NO_DEFAULT_PATH PATHS ${MY_CUSTOM_PATH}/ixml/.libs)
	find_library(LIB_UPNP upnp REQUIRED NO_DEFAULT_PATH PATHS ${MY_CUSTOM_PATH}/upnp/.libs)
	set(LIB_UPNP_INCLUDE ${MY_CUSTOM_PATH}/upnp/inc)
	set(LIB_UPNP_SRC_INCLUDE ${MY_CUSTOM_PATH}/upnp/src/inc)
	set(LIB_IXML_INCLUDE ${MY_CUSTOM_PATH}/ixml/inc)

	target_include_directories(FuzzIxml PUBLIC
		${LIB_UPNP_INCLUDE}
		${LIB_IXML_INCLUDE})
	target_link_libraries(FuzzIxml PRIVATE
		${LIB_IXML} ${LIB_FUZZING_ENGINE})

	target_include_directories(FuzzHttp PUBLIC
		${LIB_UPNP_INCLUDE}
		${LIB_UPNP_SRC_INCLUDE}
		${LIB_IXML_INCLUDE})
	target_link_libraries(FuzzHttp PRIVATE
		${LIB_UPNP} ${LIB_IXML} ${LIB_FUZZING_ENGINE})
endif()
