# LLaMaPUn Tasks -- executables
add_executable(language_detector language_detector.c)
target_link_libraries(language_detector utils language_detection LIBTEXTCAT_textcat dnmlib ${LIBXML2_LIBRARIES})

add_executable(gen_TF_IDF gen_TF_IDF.c)
target_link_libraries(gen_TF_IDF utils unicode_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
