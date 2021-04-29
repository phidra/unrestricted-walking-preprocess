# this module depends on :
#   - the 'graph' module
#   - the 'gtfs' module
#   - rapidjson (used via conan, see below)

set(JSON_SOURCES
    walking_graph_serialization.cpp
    polygon_serialization.cpp
    gtfs_serialization.cpp
)

add_library(json STATIC "${JSON_SOURCES}")


# to allow that the inclusion is prefixed by "Graph" (#include "Graph/graphtypes.h"), we use parent directory as include dir :
get_filename_component(JSON_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
target_include_directories(json PUBLIC "${JSON_PARENT_DIR}")
target_include_directories(json PRIVATE "${RAPIDJSON_INCLUDE_DIR}")
target_link_libraries(json PUBLIC graph)
target_link_libraries(json PUBLIC gtfs)
