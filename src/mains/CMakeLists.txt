add_executable(bin-uwpreprocess main-uwpreprocess.cpp)
target_link_libraries(bin-uwpreprocess PUBLIC graphlib)
target_link_libraries(bin-uwpreprocess PUBLIC gtfs_parsing)
target_link_libraries(bin-uwpreprocess PRIVATE json)
