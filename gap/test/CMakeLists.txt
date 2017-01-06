cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

project(gap_tests)

# HttpClient
add_executable(gap_http http.cpp)
target_link_libraries(gap_http PRIVATE gap)

# Github version check
add_executable(gap_lastversion lastversion.cpp)
target_link_libraries(gap_lastversion PRIVATE gap)
