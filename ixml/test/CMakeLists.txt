enable_language(CXX)

file(GLOB XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml)

ixml_add_unit_test(test-ixml test_document.c ${XML_FILES})
ixml_add_unit_test(test-ixml-poc-gh-249 poc_gh_249.c "")
ixml_add_unit_test(test-ixml-poc-gh-506 poc_gh_506.c "")
ixml_add_unit_test(test-ixml-poc-gh-517 poc_gh_517.cpp "")
ixml_add_unit_test(test-ixml-poc-ghsa-hcx4 poc_ghsa_hcx4.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-hcx4-utf8 poc_ghsa_hcx4_utf8.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-j247-print poc_ghsa_j247_print.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-j247-getbytagname poc_ghsa_j247_getbytagname.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-j247-getbytagnamens poc_ghsa_j247_getbytagnamens.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-j247-clonenode poc_ghsa_j247_clonenode.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-j247-isancestor poc_ghsa_j247_isancestor.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-q54q poc_ghsa_q54q.c "")

if(NOT UPNP_ENABLE_TESTING_INTEGRATION)
	return()
endif()

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
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-DIXML_ROOT=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
		-G "${CMAKE_GENERATOR}"
		-S${CMAKE_CURRENT_BINARY_DIR}/findpackage_wrong_components_test)

set_tests_properties(ixml_find_package_wrong_components_configure
	PROPERTIES
		FIXTURES_REQUIRED ixml_install
		WILL_FAIL TRUE)

#
# add_subdirectory_test_shared
#
if(UPNP_BUILD_SHARED)
	add_test(NAME ixml_add_subdirectory_configure_shared
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_shared
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_SHARED=ON
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test)

	add_test(NAME ixml_add_subdirectory_build_shared
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_shared)

	set_tests_properties(ixml_add_subdirectory_configure_shared
		PROPERTIES
			FIXTURES_REQUIRED ixml_install
			FIXTURES_SETUP ixml_as_config_shared)

	set_tests_properties(ixml_add_subdirectory_build_shared
		PROPERTIES
		FIXTURES_REQUIRED ixml_as_config_shared)
endif()

#
# add_subdirectory_test_static
#
if(UPNP_BUILD_STATIC)
	add_test(NAME ixml_add_subdirectory_configure_static
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_static
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_STATIC=ON
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test)

	add_test(NAME ixml_add_subdirectory_build_static
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_test_build_static)

	set_tests_properties(ixml_add_subdirectory_configure_static
		PROPERTIES
			FIXTURES_REQUIRED ixml_install
			FIXTURES_SETUP ixml_as_config_static)

	set_tests_properties(ixml_add_subdirectory_build_static
		PROPERTIES
		FIXTURES_REQUIRED ixml_as_config_static)
endif()

#
# add_subdirectory_exclude_test_shared
#
if(UPNP_BUILD_STATIC)
	add_test(NAME ixml_add_subdirectory_exclude_configure_shared
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_shared
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_SHARED=ON
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test)

	add_test(NAME ixml_add_subdirectory_exclude_build_shared
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY
			${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_shared)


	set_tests_properties(ixml_add_subdirectory_exclude_configure_shared
		PROPERTIES
			FIXTURES_REQUIRED ixml_install
			FIXTURES_SETUP ixml_asx_config_shared)

	set_tests_properties(ixml_add_subdirectory_exclude_build_shared
		PROPERTIES
			FIXTURES_REQUIRED ixml_asx_config_shared)
endif()

#
# add_subdirectory_exclude_test_static
#
if(UPNP_BUILD_STATIC)
	add_test(NAME ixml_add_subdirectory_exclude_configure_static
		COMMAND ${CMAKE_COMMAND}
			${PLATFORM}
			-B${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_static
			-DCMAKE_BUILD_TYPE=$<CONFIG>
			-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
			-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/ixml_test_install
			-DDOWNLOAD_AND_BUILD_DEPS=${DOWNLOAD_AND_BUILD_DEPS}
			-DBUILD_TESTING=OFF
			-DUPNP_BUILD_STATIC=ON
			-G "${CMAKE_GENERATOR}"
			-S${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test)

	add_test(NAME ixml_add_subdirectory_exclude_build_static
		COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
		WORKING_DIRECTORY
			${CMAKE_CURRENT_BINARY_DIR}/add_subdirectory_exclude_test_build_static)


	set_tests_properties(ixml_add_subdirectory_exclude_configure_static
		PROPERTIES
			FIXTURES_REQUIRED ixml_install
			FIXTURES_SETUP ixml_asx_config_static)

	set_tests_properties(ixml_add_subdirectory_exclude_build_static
		PROPERTIES
			FIXTURES_REQUIRED ixml_asx_config_static)
endif()