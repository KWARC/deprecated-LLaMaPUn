# LLaMaPUn Tasks -- executables
add_executable(language_detector language_detector.c)
target_link_libraries(language_detector utils language_detection LIBTEXTCAT_textcat dnmlib ${LIBXML2_LIBRARIES})

set_source_files_properties(parallel_language_detector.c PROPERTIES COMPILE_FLAGS " -pthread -Wl,-rpath -Wl,/usr/lib/openmpi -Wl,--enable-new-dtags -L/usr/lib/openmpi -lmpi ")
add_executable(parallel_language_detector parallel_language_detector.c)
target_link_libraries(parallel_language_detector utils language_detection LIBTEXTCAT_textcat dnmlib ${LIBXML2_LIBRARIES})

add_executable(gen_TF_IDF gen_TF_IDF.c)
target_link_libraries(gen_TF_IDF m utils unicode_normalizer word_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(gen_IDF gen_IDF.c)
target_link_libraries(gen_IDF m utils unicode_normalizer word_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

include_directories(${crfsuite_includepath})
add_executable(gen_libsvm_input gen_libsvm_input.c)
target_link_libraries(gen_libsvm_input utils unicode_normalizer word_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(gen_rnnlm_input gen_rnnlm_input.c)
target_link_libraries(gen_rnnlm_input utils unicode_normalizer word_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(extract_ngrams extract_ngrams.c)
target_link_libraries(extract_ngrams utils document_loader dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
