cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")

project(my-custom-ultra-usage)

# uncomment (and adapt if necessary) to use iwyu (https://include-what-you-use.org/) :
# note : there is currently a slight bug in iwyu, that causes unnecessary long paths, such as :
#       #include "DataStructures/RAPTOR/Entities/../../../Helpers/IO/Serialization.h"
# see https://github.com/include-what-you-use/include-what-you-use/issues/532
# the benefits seem greater than this bug, though, so iwyu is kept
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/lib/llvm-10/lib/clang/10.0.0/include ")
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "/usr/bin/iwyu;-Xiwyu;--no_comments;-Xiwyu;--cxx17ns")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(cmake/cppgtfs.cmake)

add_subdirectory(gtfs)
add_subdirectory(graph)
add_subdirectory(mains)

include(cmake/download-bordeaux-data-gtfs.cmake)
include(cmake/download-bordeaux-data-osm.cmake)
include(cmake/download-idf-data-gtfs.cmake)
include(cmake/download-idf-data-osm.cmake)
