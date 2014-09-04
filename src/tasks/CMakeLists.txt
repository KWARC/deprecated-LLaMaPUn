# LLaMaPUn Tasks -- executables
add_executable(language_detector language_detector.c)
target_link_libraries(language_detector utils language_detection LIBTEXTCAT_textcat dnmlib ${LIBXML2_LIBRARIES})

add_executable(gen_TF_IDF gen_TF_IDF.c)
target_link_libraries(gen_TF_IDF m utils unicode_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

include_directories(${crfsuite_includepath})
add_executable(train_DD train_DD.c)
target_link_libraries(train_DD m utils unicode_normalizer stemmer tokenizer dnmlib crf ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(find_ngrams find_ngrams.c)
target_link_libraries(find_ngrams m utils unicode_normalizer stemmer tokenizer language_detection dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
