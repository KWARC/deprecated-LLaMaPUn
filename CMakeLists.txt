project(LLaMaPUn C)
cmake_minimum_required(VERSION 2.8)
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/src/scripts)

SET(CMAKE_C_COMPILER mpicc)

# Compiler flags (fPIC required for packaging)
SET(COMMON_FLAGS
    "-O0 -g -Wall -W -Wextra -fPIC")
SET(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")

#configuration

execute_process(COMMAND sh config.sh WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/scripts)


#find the required libraries
find_library(uthash uthash)
find_library(pcre pcre)
FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
find_package(Json REQUIRED)
#find_path(JSON_INCLUDE_DIR NAMES json-c/json.h json/json.h)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR} ${JSON_INCLUDE_DIRS}/json-c ${ICONV_INCLUDE_DIR})
include_directories("third-party")
include_directories("third-party/libtextcat-2.2/src")
include_directories("third-party/morpha")
include_directories("third-party/senna")
include_directories("src")
add_subdirectory("third-party")
set(crfsuite_libpath ${CMAKE_CURRENT_BINARY_DIR}/third-party/crfsuite_project-prefix/lib)
set(crfsuite_includepath ${CMAKE_CURRENT_BINARY_DIR}/third-party/crfsuite_project-prefix/include)

add_subdirectory("src/llamapun")
add_subdirectory("src/tasks")
add_subdirectory("src/scripts")

enable_testing()
add_subdirectory("test")
