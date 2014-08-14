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

#find the required libraries
find_library(uthash uthash)
FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
find_package(Json REQUIRED)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR} ${JSON_INCLUDE_DIRS} ${ICONV_INCLUDE_DIR})

include_directories("third-party")

#add libraries
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
add_library(llamapun_para_discr llamapun_para_discr.c)
add_library(unicode_normalizer unicode_normalizer.c)
add_library(llamapun_gt_estimate llamapun_gt_estimate.c)
#add_library(dnmlib dnmlib.c)
add_library(old_dnmlib old_dnmlib.c)
#add_library(preprocessor preprocessor.c)

#special compiler settings:
#morpha yields many warnings
set_source_files_properties(stemmer.c PROPERTIES COMPILE_FLAGS " -w ")
#required for parsing json
set_source_files_properties(llamapun_gt_estimate.c PROPERTIES COMPILE_FLAGS " -std=c99 ")

#link libraries
target_link_libraries(SENNA_Hash SENNA_utils)
target_link_libraries(SENNA_Tokenizer SENNA_Hash SENNA_utils)
target_link_libraries(SENNA_nn SENNA_utils)
target_link_libraries(SENNA_POS SENNA_utils SENNA_Hash SENNA_nn)
target_link_libraries(llamapun_senna_pos SENNA_utils SENNA_Hash SENNA_nn SENNA_Tokenizer SENNA_POS
                                        llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(llamapun_ngrams stopwords stemmer old_dnmlib unicode_normalizer
                                        llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(llamapun_gt_estimate ${JSON_LIBRARIES} ${uthash_LIBRARIES})
target_link_libraries(llamapun_para_discr old_dnmlib llamapun_utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
#target_link_libraries(dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(old_dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
#target_link_libraries(preprocessor ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(unicode_normalizer ${ICONV_LIBRARIES})

#tests
add_executable(TestSENNA tests/senna_post_test.c)
target_link_libraries(TestSENNA llamapun_senna_pos ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(TestSTOPWORDS tests/stopwords_test.c)
target_link_libraries(TestSTOPWORDS stopwords ${JSON_LIBRARIES} ${uthash_LIBRARIES})

add_executable(TestSTEMMER tests/stemmer_test.c)
target_link_libraries(TestSTEMMER stemmer)

add_executable(TestNGRAMS tests/ngrams_test.c)
target_link_libraries(TestNGRAMS llamapun_ngrams ${JSON_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(TestPARA_DISCR tests/para_discr_test.c)
target_link_libraries(TestPARA_DISCR llamapun_para_discr ${JSON_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(TestUnicodeNormalizer tests/unicode_normalizer_test.c)
target_link_libraries(TestUnicodeNormalizer unicode_normalizer)


enable_testing()
add_test(SENNA_POS_Test TestSENNA WORKING_DIRECTORY "tests")
add_test(STOPWORDS_Test TestSTOPWORDS WORKING_DIRECTORY "tests")
add_test(STEMMER_Test TestSTEMMER WORKING_DIRECTORY "tests")
add_test(NGRAMS_Test TestNGRAMS WORKING_DIRECTORY "tests")
add_test(PARA_DISCR_Test TestPARA_DISCR WORKING_DIRECTORY "tests")
#add_test(DNMLIB_Test TestDNMLIB WORKING_DIRECTORY "tests")
add_test(UNICODE_NORMALIZER_Test TestUnicodeNormalizer WORKING_DIRECTORY "tests")


#installation of all .c and .h files 

file(GLOB llamapun_SOURCES *.c)
file(GLOB llamapun_HEADERS *.h)
add_library(llamapun SHARED ${llamapun_SOURCES} ${llamapun_HEADERS})

target_link_libraries(llamapun SENNA_utils SENNA_Hash SENNA_nn SENNA_Tokenizer SENNA_POS
	${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
install(TARGETS llamapun DESTINATION /usr/lib)
install(FILES ${llamapun_HEADERS} DESTINATION /usr/include/llamapun)
