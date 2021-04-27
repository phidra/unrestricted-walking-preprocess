add_executable(build-preparatory build-preparatory.cpp)
target_link_libraries(build-preparatory PUBLIC graphlib)
target_link_libraries(build-preparatory PUBLIC gtfs_parsing)
