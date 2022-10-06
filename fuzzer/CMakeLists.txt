add_executable(FuzzIxml FuzzIxml.c)
target_link_libraries(FuzzIxml ixml_static ${LIB_FUZZING_ENGINE})
