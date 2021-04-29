add_executable(bin-uwpreprocess main-uwpreprocess.cpp)
target_link_libraries(bin-uwpreprocess PUBLIC graph)
target_link_libraries(bin-uwpreprocess PUBLIC gtfs)
target_link_libraries(bin-uwpreprocess PRIVATE json)
