# this lib depends on :
#   - the cmake target 'ad_cppgtfs' (for the library cppgtfs)

# this module has no other dependency, and particularly, it does NOT depend on ULTRA

set(GTFSPARSING_SOURCES
    gtfs_parsing_structures.cpp
    gtfs_parsed_data.cpp
)

add_library(gtfs STATIC "${GTFSPARSING_SOURCES}")
target_include_directories(gtfs PRIVATE "${CPPGTFS_INCLUDE_DIR}")
target_link_libraries(gtfs PRIVATE ad_cppgtfs)
