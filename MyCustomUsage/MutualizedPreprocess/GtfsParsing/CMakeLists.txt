# this lib depends on :
#   - the cmake target 'ad_cppgtfs' (for the library cppgtfs)
#   - rapidjson (used via conan, see below)

# this module has no other dependency, and particularly, it does NOT depend on ULTRA

set(GTFSPARSING_SOURCES
    gtfs_parsing_structures.cpp
    gtfs_parsed_data.cpp
    gtfs_geojson.cpp
)

add_library(gtfs_parsing STATIC "${GTFSPARSING_SOURCES}")
target_include_directories(gtfs_parsing PRIVATE "${CPPGTFS_INCLUDE_DIR}")
target_include_directories(gtfs_parsing PRIVATE "${CONAN_INCLUDE_DIRS_RAPIDJSON}")
target_link_libraries(gtfs_parsing PRIVATE ad_cppgtfs)
