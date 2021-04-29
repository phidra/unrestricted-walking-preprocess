cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")

project(unrestricted-walking-preprocess)

# uncomment (and adapt if necessary) to use iwyu (https://include-what-you-use.org/) :
# note : there is currently a slight bug in iwyu, that causes unnecessary long paths, such as :
#       #include "DataStructures/RAPTOR/Entities/../../../Helpers/IO/Serialization.h"
# see https://github.com/include-what-you-use/include-what-you-use/issues/532
# the benefits seem greater than this bug, though, so iwyu is kept
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/lib/llvm-10/lib/clang/10.0.0/include ")
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "/usr/bin/iwyu;-Xiwyu;--no_comments;-Xiwyu;--cxx17ns")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# if we are the main project, we use conan to manage dependency to rapidjson
# otherwise, we expect the caller to set the variable RAPIDJSON_INCLUDE_DIR
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Using conan to define RAPIDJSON_INCLUDE_DIR")
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
    set(RAPIDJSON_INCLUDE_DIR "${CONAN_INCLUDE_DIRS_RAPIDJSON}")
else()
    message(STATUS "Expecting the caller to define RAPIDJSON_INCLUDE_DIR")
endif()
message(STATUS "To include rapidjson, the following variable is used :")
message(STATUS "RAPIDJSON_INCLUDE_DIR=${RAPIDJSON_INCLUDE_DIR}")

include(cmake/cppgtfs.cmake)

add_subdirectory(gtfs)
add_subdirectory(graph)
add_subdirectory(json)
add_subdirectory(mains)

include(cmake/download-bordeaux-data-gtfs.cmake)
include(cmake/download-bordeaux-data-osm.cmake)
include(cmake/download-idf-data-gtfs.cmake)
include(cmake/download-idf-data-osm.cmake)
