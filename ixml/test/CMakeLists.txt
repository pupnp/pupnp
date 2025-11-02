file(GLOB XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml)

ixml_add_unit_test(test-ixml test_document.c ${XML_FILES})
ixml_add_unit_test(test-ixml-poc-gh-506 poc_gh_506.c "")
