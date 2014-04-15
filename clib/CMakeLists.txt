project(LLaMaPUn C)
cmake_minimum_required(VERSION 2.8)
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )

# Compiler flags
SET(COMMON_FLAGS
    "-Wall -W -Wextra")
SET(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")

find_library(LIBJSON json-c)
find_library(uthash uthash)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

include_directories("third-party")

add_library(SENNA_utils third-party/senna/SENNA_utils.c)
add_library(SENNA_nn third-party/senna/SENNA_nn.c)
add_library(SENNA_POS third-party/senna/SENNA_POS.c)
add_library(llamapun_utils llamapun_utils.c)
add_library(llamapun_senna_pos llamapun_senna_pos.c)

add_executable(TestSENNA tests/senna_post_test.c)
target_link_libraries(TestSENNA llamapun_utils llamapun_senna_pos ${LIBJSON} ${uthash_LIBRARIES} ${LIBXML2_LIBRARIES})

enable_testing()
add_test(SENNA_POS_Test TestSENNA WORKING_DIRECTORY "tests")
