enable_language(CXX)

file(GLOB XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml)

ixml_add_unit_test(test-ixml test_document.c ${XML_FILES})
ixml_add_unit_test(test-ixml-poc-gh-249 poc_gh_249.c "")
ixml_add_unit_test(test-ixml-poc-gh-506 poc_gh_506.c "")
ixml_add_unit_test(test-ixml-poc-gh-517 poc_gh_517.cpp "")
ixml_add_unit_test(test-ixml-poc-ghsa-hcx4 poc_ghsa_hcx4.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-hcx4-utf8 poc_ghsa_hcx4_utf8.c "")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/findpackage_test)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/find_package_no_components_test)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/find_package_wrong_components_test)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/find_package_test.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/findpackage_test/CMakeLists.txt @ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/find_package_no_components.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/findpackage_no_components_test/CMakeLists.txt
	@ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/find_package_wrong_components.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test/CMakeLists.txt
	@ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test/CMakeLists.txt @ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory_exclude.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test/CMakeLists.txt
	@ONLY)

add_test(NAME ixml_install
	COMMAND ${CMAKE_COMMAND}
		--install ${PUPNP_BINARY_DIR}
		--prefix ${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		--config $<CONFIG>
    WORKING_DIRECTORY ${PUPNP_BINARY_DIR})

set_tests_properties(ixml_install
	PROPERTIES FIXTURES_SETUP ixml_install)

# CMAKE_GENERATOR_PLATFORM doesn't work in the if
set(GENERATOR ${CMAKE_GENERATOR_PLATFORM})

if(GENERATOR)
	set(PLATFORM "-A ${GENERATOR}")
endif(GENERATOR)

#
# findpackage_test
#
add_test(NAME ixml_find_package_configure
	COMMAND ${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/findpackage_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-DIXML_ROOT=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_test)

add_test(NAME ixml_find_package_build
	COMMAND ${CMAKE_COMMAND}
		--build .
		--config $<CONFIG>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/findpackage_test_build)

set_tests_properties(ixml_find_package_configure
	PROPERTIES
		FIXTURES_REQUIRED ixml_install
		FIXTURES_SETUP ixml_fp_config)

set_tests_properties(ixml_find_package_build
	PROPERTIES
		FIXTURES_REQUIRED ixml_fp_config
		FIXTURES_SETUP ixml_fp_build)

#
# findpackage_no_components_test
#
add_test(NAME ixml_find_package_no_components_configure
	COMMAND ${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/findpackage_no_components_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-DIXML_ROOT=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_no_components_test)

set_tests_properties(ixml_find_package_no_components_configure
	PROPERTIES
		FIXTURES_REQUIRED ixml_install)

#
# findpackage_no_component_test
#
add_test(NAME ixml_find_package_wrong_components_configure
	COMMAND	${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-DIXML_ROOT=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test)

set_tests_properties(ixml_find_package_wrong_components_configure
	PROPERTIES
		FIXTURES_REQUIRED ixml_install
		WILL_FAIL TRUE)

#
# add_subdirectory_test
#
add_test(NAME ixml_add_subdirectory_configure
	COMMAND ${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test)

add_test(NAME ixml_add_subdirectory_build
	COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build)

set_tests_properties(ixml_add_subdirectory_configure
	PROPERTIES
		FIXTURES_REQUIRED ixml_install
		FIXTURES_SETUP ixml_as_config)

set_tests_properties(ixml_add_subdirectory_build
	PROPERTIES
	FIXTURES_REQUIRED as_config)

#
# add_subdirectory_exclude_test
#
add_test(NAME ixml_add_subdirectory_exclude_configure
	COMMAND ${CMAKE_COMMAND}
		${PLATFORM}
		-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build
		-DCMAKE_BUILD_TYPE=$<CONFIG>
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test)

add_test(NAME ixml_add_subdirectory_exclude_build
	COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
	WORKING_DIRECTORY
		${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build)


set_tests_properties(ixml_add_subdirectory_exclude_configure
	PROPERTIES
		FIXTURES_REQUIRED ixml_install
		FIXTURES_SETUP ixml_asx_config)

set_tests_properties(ixml_add_subdirectory_exclude_build
	PROPERTIES
		FIXTURES_REQUIRED ixml_as_config)
