function (addGMock testName sourceFile)
	cmake_parse_arguments (PARSE_ARGV 2 "agm" "" "" "ADDITIONAL_INCLUDE_DIRS")

	if (agm_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Additional Arg given to ${testName}: ${agm_UNPARSED_ARGUMENTS}")
	endif()

	addTestExecutable (${testName} ${sourceFile})
	if (agm_ADDITIONAL_INCLUDE_DIRS)
		target_include_directories (${testName}
			PRIVATE ${agm_ADDITIONAL_INCLUDE_DIRS}
		)

		target_include_directories (${testName}-static
			PRIVATE ${agm_ADDITIONAL_INCLUDE_DIRS}
		)
	endif()

	target_link_libraries (${testName}
		PRIVATE GTest::gmock
	)

	target_link_libraries (${testName}-static
		PRIVATE GTest::gmock
	)


	gtest_add_tests (TARGET ${testName}
		TEST_PREFIX test-upnp-
		TEST_LIST GTEST_${testName}
	)

	if (WIN32)
		findTestEnv (${testName} TEST_ENV)

		set_tests_properties (${GTEST_${testName}} PROPERTIES
			ENVIRONMENT "${TEST_ENV}"
		)
	endif()

	gtest_add_tests (TARGET ${testName}-static
		TEST_PREFIX test-upnp-
		TEST_SUFFIX -static
	)

	findTestEnv (${testName} TEST_ENV)
endfunction()

function (addTestExecutable testName sourceFile)
	add_executable (${testName}
		${sourceFile}
	)

	target_link_libraries (${testName}
		PRIVATE upnp_shared
	)

	if (HAVE_MACRO_PREFIX_MAP)
		target_compile_options(${testName}
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()

	add_executable (${testName}-static
		${sourceFile}
	)

	target_link_libraries (${testName}-static
		PRIVATE upnp_static
	)

	if (HAVE_MACRO_PREFIX_MAP)
		target_compile_options(${testName}-static
			PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		)
	endif()
endfunction()

function (addUnitTest testName sourceFile)
	addTestExecutable (${testName} ${sourceFile})

	add_test (NAME ${testName}
		COMMAND ${testName}
	)

	if (WIN32)
		findTestEnv (${testName} TEST_ENV)

		set_tests_properties (${testName} PROPERTIES
			ENVIRONMENT "${TEST_ENV}"
		)
	endif()

	add_test (NAME ${testName}-static
		COMMAND ${testName}-static
	)
endfunction()

function (findTestEnv testName resultVar)
	findTestLibs (${testName} ${resultVar})
	set (tempEnv "PATH=")

	foreach (entry IN ITEMS ${${resultVar}})
		string (APPEND tempEnv "${entry}\\\;")
	endforeach()

	string (APPEND tempEnv "%PATH%")
	set (${resultVar} ${tempEnv} PARENT_SCOPE)
endfunction()

function (findTestLibs testName resultVar)
	unset (linkLibs)

	if (NOT TARGET ${testName})
		set (interface TRUE)
	else()
		get_property (interface
			TARGET ${testName}
			PROPERTY IMPORTED
		)
	endif()

	if (NOT ${interface})
		get_property (linkLibs
			TARGET ${testName}
			PROPERTY LINK_LIBRARIES
		)

		foreach (lib IN ITEMS ${linkLibs})
			findTestLibs (${lib} ${resultVar})

			if (NOT TARGET ${lib})
				set (interface2 TRUE)
			else()
				get_property (interface2
					TARGET ${lib}
					PROPERTY IMPORTED
				)
			endif()

			if (NOT ${interface2})
				list (FIND ${resultVar} "$<TARGET_FILE_DIR:${lib}>" index)

				if (${index} STREQUAL "-1")
					list (APPEND ${resultVar} "$<TARGET_FILE_DIR:${lib}>")
				endif()
			endif()
		endforeach()
	endif()

	set (${resultVar} ${${resultVar}} PARENT_SCOPE)
endfunction()
