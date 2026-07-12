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

