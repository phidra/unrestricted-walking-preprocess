# this lib depends on :
#   - BoostGeometry (expected to be available in system libs)
#   - libosmium (expected to be available in system libs)
#   - a few other libs expected to be availablein system libs, see below

# this module has no other dependency, and particularly, it does NOT depend on ULTRA

set(GRAPH_SOURCES
    polygon.cpp
    extending_with_stops.cpp
    osmparsing.cpp
    graph.cpp
    walking_graph.cpp
)

add_library(graph STATIC "${GRAPH_SOURCES}")
target_include_directories(graph PRIVATE "${CPPGTFS_INCLUDE_DIR}")
target_link_libraries(graph PRIVATE ad_cppgtfs)


# to allow that the inclusion is prefixed by "Graph" (#include "Graph/graphtypes.h"), we use parent directory as include dir :
get_filename_component(GRAPH_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
target_include_directories(graph PUBLIC "${GRAPH_PARENT_DIR}")

set(LIBOSMIUM_LINK_DEPS bz2 z expat -pthread)
target_link_libraries(graph PRIVATE "${LIBOSMIUM_LINK_DEPS}")
