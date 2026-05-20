file(GLOB XML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/testdata/*.xml)

ixml_add_unit_test(test-ixml test_document.c ${XML_FILES})
ixml_add_unit_test(test-ixml-poc-gh-249 poc_gh_249.c "")
ixml_add_unit_test(test-ixml-poc-gh-506 poc_gh_506.c "")
ixml_add_unit_test(test-ixml-poc-gh-517 poc_gh_517.cpp "")
ixml_add_unit_test(test-ixml-poc-ghsa-hcx4 poc_ghsa_hcx4.c "")
ixml_add_unit_test(test-ixml-poc-ghsa-hcx4-utf8 poc_ghsa_hcx4_utf8.c "")
