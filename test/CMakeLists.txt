# Test harness
add_executable(TestSENNA senna_post_test.c)
target_link_libraries(TestSENNA llamapun_senna_pos ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(TestSTOPWORDS stopwords_test.c)
target_link_libraries(TestSTOPWORDS stopwords ${JSON_LIBRARIES} ${uthash_LIBRARIES})

add_executable(TestSTEMMER stemmer_test.c)
target_link_libraries(TestSTEMMER stemmer)

add_executable(TestNGRAMS ngrams_test.c)
target_link_libraries(TestNGRAMS ngrams ${JSON_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(TestUnicodeNormalizer unicode_normalizer_test.c)
target_link_libraries(TestUnicodeNormalizer unicode_normalizer)

add_executable(TestTokenization tokenization_test.c)
target_link_libraries(TestTokenization dnmlib tokenizer unicode_normalizer ${LIBXML2_LIBRARIES})

add_executable(TestLanguage language_test.c)
target_link_libraries(TestLanguage language_detection)

add_executable(TestCombined combined_test.c)
target_link_libraries(TestCombined dnmlib unicode_normalizer utils tokenizer ${LIBXML2_LIBRARIES})

# Test harness


add_test(NAME SENNA_POS_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestSENNA WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME STOPWORDS_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestSTOPWORDS WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME STEMMER_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestSTEMMER WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME NGRAMS_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestNGRAMS WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME UNICODE_NORMALIZER_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestUnicodeNormalizer WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME TOKENIZATION_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestTokenization WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME LANGUAGE_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestLanguage WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
add_test(NAME COMBINED_Test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/./TestCombined WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)


ADD_TEST(NAME TestSENNA_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestSENNA WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestSTOPWORDS_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestSTOPWORDS WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestSTEMMER_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestSTEMMER WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestNGRAMS_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestNGRAMS WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestUnicodeNormalizer_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestUnicodeNormalizer WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestTokenization_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestTokenization WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestLanguage_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestLanguage WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
ADD_TEST(NAME TestCombined_leaks COMMAND sh ${CMAKE_CURRENT_LIST_DIR}/../src/scripts/leak_check.sh ${CMAKE_CURRENT_BINARY_DIR}/./TestCombined WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test_working_directory)
