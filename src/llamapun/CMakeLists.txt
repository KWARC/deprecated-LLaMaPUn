#add libraries

add_library(utils utils.c)
add_library(llamapun_senna_pos senna_pos.c)
add_library(stopwords stopwords.c)
add_library(stemmer stemmer.c)
add_library(ngrams ngrams.c)
add_library(unicode_normalizer unicode_normalizer.c)
add_library(gt_estimate gt_estimate.c)
add_library(tokenizer tokenizer.c)
add_library(dnmlib dnmlib.c)
add_library(language_detection language_detection.c)
add_library(crf crf.c)

#special compiler settings:
#morpha yields many warnings
set_source_files_properties(stemmer.c PROPERTIES COMPILE_FLAGS " -w ")
target_link_libraries(stemmer MORPHA)
#required for parsing json
set_source_files_properties(gt_estimate.c PROPERTIES COMPILE_FLAGS "-Wno-unused-but-set-variable -std=c99 ")

# External for CRFsuite
add_library( crfsuite SHARED IMPORTED )
set_target_properties( crfsuite PROPERTIES IMPORTED_LOCATION ${crfsuite_libpath}/libcrfsuite.so)
link_directories(${crfsuite_libpath})
include_directories(${crfsuite_includepath})

#link libraries
target_link_libraries(stopwords ${JSON_LIBRARIES} ${uthash_LIBRARIES})
target_link_libraries(llamapun_senna_pos SENNA_utils SENNA_Hash SENNA_nn SENNA_Tokenizer SENNA_POS
                                        utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(ngrams stopwords stemmer unicode_normalizer
                                        utils ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(gt_estimate ${JSON_LIBRARIES} ${uthash_LIBRARIES})
target_link_libraries(dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(unicode_normalizer ${ICONV_LIBRARIES} ${LIBXML2_LIBRARIES})
target_link_libraries(tokenizer stopwords dnmlib pcre SENNA_utils SENNA_Hash SENNA_nn SENNA_Tokenizer)
target_link_libraries(language_detection LIBTEXTCAT_textcat)
target_link_libraries(crf crfsuite)

#installation of all .c and .h files 
file(GLOB llamapun_SOURCES *.c)
file(GLOB llamapun_HEADERS *.h)
add_library(llamapun SHARED ${llamapun_SOURCES} ${llamapun_HEADERS})

target_link_libraries(llamapun SENNA_utils SENNA_Hash SENNA_nn SENNA_Tokenizer SENNA_POS
  ${JSON_LIBRARIES} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES} pcre)
install(TARGETS llamapun DESTINATION /usr/lib)
install(FILES ${llamapun_HEADERS} DESTINATION /usr/include/llamapun)
