project(LLaMaPUn C)
cmake_minimum_required(VERSION 2.8)
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

# Compiler flags (fPIC required for packaging)
SET(COMMON_FLAGS
    "-Wall -W -Wextra -fPIC")
SET(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")

find_library(uthash uthash)
FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
find_package(Json REQUIRED)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR} ${JSON_INCLUDE_DIRS} ${ICONV_INCLUDE_DIR})

include_directories("third-party")

add_library(SENNA_utils third-party/senna/SENNA_utils.c)
add_library(SENNA_Hash third-party/senna/SENNA_Hash.c)
add_library(SENNA_nn third-party/senna/SENNA_nn.c)
add_library(SENNA_POS third-party/senna/SENNA_POS.c)
add_library(SENNA_Tokenizer third-party/senna/SENNA_Tokenizer.c)
add_library(llamapun_utils llamapun_utils.c)
add_library(llamapun_senna_pos llamapun_senna_pos.c)
add_library(stopwords stopwords.c)
add_library(stemmer stemmer.c)
add_library(llamapun_ngrams llamapun_ngrams.c)
add_library(unicode_normalizer unicode_normalizer.c)
#add_library(ngram_merge ngram_merge.c)

#morpha yields many warnings
set_source_files_properties(stemmer.c PROPERTIES COMPILE_FLAGS " -w ")
#required for parsing json
set_source_files_properties(ngram_merge.c PROPERTIES COMPILE_FLAGS " -std=c99 ")

target_link_libraries(SENNA_Hash SENNA_utils)
target_link_libraries(SENNA_Tokenizer SENNA_Hash SENNA_utils)
target_link_libraries(SENNA_nn SENNA_utils)
target_link_libraries(SENNA_POS SENNA_utils SENNA_Hash SENNA_nn)
target_link_libraries(llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(llamapun_senna_pos
                                        SENNA_utils SENNA_Hash SENNA_nn SENNA_Tokenizer SENNA_POS
                                        llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(llamapun_ngrams stopwords stemmer
                                        llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
add_executable(TestSENNA tests/senna_post_test.c)
target_link_libraries(TestSENNA llamapun_senna_pos ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
add_executable(TestSTOPWORDS tests/stopwords_test.c)
target_link_libraries(TestSTOPWORDS stopwords ${JSON_LIBRARIES} ${uthash_LIBRARIES})
add_executable(TestSTEMMER tests/stemmer_test.c)
target_link_libraries(TestSTEMMER stemmer)
add_executable(TestNGRAMS tests/ngrams_test.c)
target_link_libraries(TestNGRAMS llamapun_ngrams unicode_normalizer ${ICONV_LIBRARIES} ${JSON_LIBRARIES} ${LIBXML2_LIBRARIES})
add_executable(TestUnicodeNormalizer tests/unicode_normalizer_test.c)
target_link_libraries(TestUnicodeNormalizer unicode_normalizer ${ICONV_LIBRARIES})

enable_testing()
add_test(SENNA_POS_Test TestSENNA WORKING_DIRECTORY "tests")
add_test(STOPWORDS_Test TestSTOPWORDS WORKING_DIRECTORY "tests")
add_test(STEMMER_Test TestSTEMMER WORKING_DIRECTORY "tests")
add_test(NGRAMS_Test TestNGRAMS WORKING_DIRECTORY "tests")
add_test(UNICODE_NORMALIZER_Test TestUnicodeNormalizer WORKING_DIRECTORY "tests")


#install

file(GLOB llamapun_SOURCES *.c)
file(GLOB llamapun_HEADERS *.h)
add_library(llamapun SHARED ${llamapun_SOURCES} ${llamapun_HEADERS})

target_link_libraries(llamapun ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
install(TARGETS llamapun DESTINATION lib)
install(FILES ${llamapun_HEADERS} DESTINATION include/llamapun)
