cmake_minimum_required(VERSION 3.11)

project(fuzzer LANGUAGES C)
add_executable(FuzzIxml FuzzIxml.c)
add_executable(FuzzHttp FuzzHttp.c)
add_executable(FuzzServiceTable FuzzServiceTable.c)
add_executable(FuzzIxmlDom FuzzIxmlDom.c)
add_executable(FuzzIxmlOps FuzzIxmlOps.c)

# OSS-Fuzz takes <target>.dict and <target>.options from beside the binary, and
# its build.sh only copies Fuzz* from this build directory: put them here.
file(GLOB FUZZER_AUX_FILES *.dict *.options)
foreach(aux_file ${FUZZER_AUX_FILES})
	get_filename_component(aux_name ${aux_file} NAME)
	get_filename_component(aux_target ${aux_file} NAME_WE)
	if(NOT TARGET ${aux_target})
		message(WARNING "fuzzer/${aux_name}: no fuzz target is named ${aux_target}, the fuzzing engine will ignore this file")
	endif()
	configure_file(${aux_file} ${aux_name} COPYONLY)
endforeach()

target_link_libraries(FuzzIxml ixml_static ${LIB_FUZZING_ENGINE})
target_include_directories(FuzzHttp PRIVATE ${CMAKE_SOURCE_DIR}/upnp/src/threadutil)
target_link_libraries(FuzzHttp upnp_static ${LIB_FUZZING_ENGINE})
target_include_directories(FuzzServiceTable PRIVATE ${CMAKE_SOURCE_DIR}/upnp/src/threadutil)
target_link_libraries(FuzzServiceTable upnp_static ${LIB_FUZZING_ENGINE})
target_link_libraries(FuzzIxmlDom ixml_static ${LIB_FUZZING_ENGINE})
target_link_libraries(FuzzIxmlOps ixml_static ${LIB_FUZZING_ENGINE})
