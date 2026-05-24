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
	NAME test-invariant-tv-device
	COMMAND ${Python3_EXECUTABLE} -m pytest
		${CMAKE_CURRENT_SOURCE_DIR}/test_invariant_tv_device.py
		-v
)
