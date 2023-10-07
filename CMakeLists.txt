cmake_minimum_required(VERSION 3.26)
project(2006_VRPTW_Pareto)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(libraries/BLT)

file(GLOB_RECURSE VRPTW_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

include_directories(include/)

add_executable(2006_VRPTW_Pareto ${VRPTW_BUILD_FILES})

target_link_libraries(2006_VRPTW_Pareto BLT)
