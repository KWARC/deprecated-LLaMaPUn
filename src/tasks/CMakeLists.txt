# LLaMaPUn Tasks -- executables
add_executable(language_detector language_detector.c)
target_link_libraries(language_detector utils language_detection LIBTEXTCAT_textcat dnmlib ${LIBXML2_LIBRARIES})

add_executable(gen_TF_IDF gen_TF_IDF.c)
target_link_libraries(gen_TF_IDF m utils unicode_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

add_executable(train_DD train_DD.c)
add_dependencies(train_DD CRFpp)

# External for CRFsuite
add_library( crfsuite SHARED IMPORTED )
set_target_properties( crfsuite PROPERTIES IMPORTED_LOCATION ${crfsuite_libpath}/libcrfsuite.so)
link_directories(${crfsuite_libpath})
include_directories(${crfsuite_includepath})
# Link into train_DD
target_link_libraries(train_DD crfsuite m utils unicode_normalizer stemmer tokenizer dnmlib ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})
