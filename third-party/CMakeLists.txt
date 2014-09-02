#libtextcat
add_library(SENNA_utils senna/SENNA_utils.c)
add_library(SENNA_Hash senna/SENNA_Hash.c)
add_library(SENNA_nn senna/SENNA_nn.c)
add_library(SENNA_POS senna/SENNA_POS.c)
add_library(SENNA_Tokenizer senna/SENNA_Tokenizer.c)

target_link_libraries(SENNA_Hash SENNA_utils)
target_link_libraries(SENNA_Tokenizer SENNA_Hash SENNA_utils)
target_link_libraries(SENNA_nn SENNA_utils)
target_link_libraries(SENNA_POS SENNA_utils SENNA_Hash SENNA_nn)

add_library(LIBTEXTCAT_common libtextcat-2.2/src/common.c)
add_library(LIBTEXTCAT_createfp libtextcat-2.2/src/createfp.c)
add_library(LIBTEXTCAT_fingerprint libtextcat-2.2/src/fingerprint.c)
add_library(LIBTEXTCAT_wg_mempool libtextcat-2.2/src/wg_mempool.c)
add_library(LIBTEXTCAT_textcat libtextcat-2.2/src/textcat.c)

target_link_libraries(LIBTEXTCAT_textcat LIBTEXTCAT_common LIBTEXTCAT_createfp LIBTEXTCAT_fingerprint LIBTEXTCAT_wg_mempool)

add_library(MORPHA morpha/morpha.yy.c)
include(ExternalProject)
ExternalProject_Add(crfsuite_project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/crfsuite-0.12
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/crfsuite-0.12/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE})

