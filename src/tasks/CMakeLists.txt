# LLaMaPUn Tasks -- executables
add_executable(language_detector language_detector.c)
target_link_libraries(language_detector utils LIBTEXTCAT_textcat dnmlib ${LIBXML2_LIBRARIES})
