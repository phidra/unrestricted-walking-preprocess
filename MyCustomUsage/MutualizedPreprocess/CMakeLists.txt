add_subdirectory(GtfsParsing)
add_subdirectory(Graph)


# allow the inclusion prefixed by "MutualizedPreprocess" (#include "MutualizedPreprocess/Graph/graphtypes.h"), by using parent dir as include dir :
get_filename_component(MUTUALIZEDPREPROCESS_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
target_include_directories(graphlib PUBLIC "${MUTUALIZEDPREPROCESS_PARENT_DIR}")
target_include_directories(gtfs_parsing PUBLIC "${MUTUALIZEDPREPROCESS_PARENT_DIR}")
