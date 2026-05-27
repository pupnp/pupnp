find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_FOUND)
	message(STATUS "Python3 not found; skipping Python tests")
	return()
endif()

execute_process(
	COMMAND ${Python3_EXECUTABLE} -m pytest --version
	RESULT_VARIABLE _pytest_result
	OUTPUT_QUIET
	ERROR_QUIET
)

if(NOT _pytest_result EQUAL 0)
	message(STATUS "pytest not found; skipping Python tests")
	return()
endif()

add_test(
	NAME test-init-py
	COMMAND ${Python3_EXECUTABLE} -m pytest
		${CMAKE_CURRENT_SOURCE_DIR}/test_init.py
		-v
)

set(_test_init_py_env "UPNP_BUILD_DIR=${CMAKE_BINARY_DIR}")
if(CMAKE_C_FLAGS MATCHES "sanitize=address" OR
		CMAKE_SHARED_LINKER_FLAGS MATCHES "sanitize=address")
	execute_process(
		COMMAND ${CMAKE_C_COMPILER} -print-file-name=libasan.so
		OUTPUT_VARIABLE _asan_path
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(_asan_path AND NOT _asan_path STREQUAL "libasan.so")
		list(APPEND _test_init_py_env "LD_PRELOAD=${_asan_path}")
	endif()
endif()

set_tests_properties(test-init-py PROPERTIES
	ENVIRONMENT "${_test_init_py_env}"
)

add_test(
	NAME test-issue-110-py
	COMMAND ${Python3_EXECUTABLE} -m pytest
		${CMAKE_CURRENT_SOURCE_DIR}/test_issue_110.py
		-v
)

set_tests_properties(test-issue-110-py PROPERTIES
	ENVIRONMENT "${_test_init_py_env}"
)

add_test(
	NAME test-issue-250-py
	COMMAND ${Python3_EXECUTABLE} -m pytest
		${CMAKE_CURRENT_SOURCE_DIR}/test_issue_250.py
		-v
)

set_tests_properties(test-issue-250-py PROPERTIES
	ENVIRONMENT "${_test_init_py_env}"
)
