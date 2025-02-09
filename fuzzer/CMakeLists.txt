cmake_minimum_required(VERSION 3.11)

project(fuzzer LANGUAGES C)
add_executable(FuzzIxml FuzzIxml.c)

set(MY_CUSTOM_PATH ..)
find_library(LIB_IXML ixml REQUIRED NO_DEFAULT_PATH PATHS ${MY_CUSTOM_PATH}/ixml/.libs)
set(LIB_UPNP_INCLUDE ${MY_CUSTOM_PATH}/upnp/inc)
set(LIB_IXML_INCLUDE ${MY_CUSTOM_PATH}/ixml/inc)

target_include_directories(FuzzIxml PUBLIC
	${LIB_UPNP_INCLUDE}
	${LIB_IXML_INCLUDE})
target_link_libraries(FuzzIxml PRIVATE
	${LIB_IXML} ${LIB_FUZZING_ENGINE})
