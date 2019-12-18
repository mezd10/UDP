cmake_minimum_required(VERSION 3.15)
project(udp)

set(CMAKE_CXX_STANDARD 14)

add_executable(udp main.cpp)

find_package(PostgreSQL REQUIRED)
target_include_directories(udp PRIVATE ${PostgreSQL_INCLUDE_DIRS})

# Add libraries to link your target againts. Again, PRIVATE is important for multi-target projects
target_link_libraries(udp PRIVATE ${PostgreSQL_LIBRARIES} pthread)