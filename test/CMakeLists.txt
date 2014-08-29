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

add_executable(TestCombined combined_test.c)
target_link_libraries(TestCombined dnmlib unicode_normalizer utils tokenizer ${LIBXML2_LIBRARIES})



add_test(SENNA_POS_Test TestSENNA WORKING_DIRECTORY "test")
add_test(STOPWORDS_Test TestSTOPWORDS WORKING_DIRECTORY "test")
add_test(STEMMER_Test TestSTEMMER WORKING_DIRECTORY "test")
add_test(NGRAMS_Test TestNGRAMS WORKING_DIRECTORY "test")
add_test(UNICODE_NORMALIZER_Test TestUnicodeNormalizer WORKING_DIRECTORY "test")
add_test(TOKENIZATION_Test TestTokenization WORKING_DIRECTORY "test")
add_test(COMBINED_Test TestCombined WORKING_DIRECTORY "test")

ADD_TEST(NAME TestSENNA_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestSENNA)
ADD_TEST(NAME TestSTOPWORDS_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestSTOPWORDS)
ADD_TEST(NAME TestSTEMMER_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestSTEMMER)
ADD_TEST(NAME TestNGRAMS_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestNGRAMS)
ADD_TEST(NAME TestUnicodeNormalizer_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestUnicodeNormalizer)
ADD_TEST(NAME TestTokenization_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestTokenization)
ADD_TEST(NAME TestCombined_leaks COMMAND sh ../../src/scripts/leak_check.sh ./TestCombined)
