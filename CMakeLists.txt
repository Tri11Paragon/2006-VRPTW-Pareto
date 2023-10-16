cmake_minimum_required(VERSION 3.24)
project(2006_VRPTW_Pareto)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(libraries/BLT)

file(GLOB_RECURSE VRPTW_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

include_directories(include/)

add_executable(2006_VRPTW_Pareto ${VRPTW_BUILD_FILES})

target_link_libraries(2006_VRPTW_Pareto BLT)

if (${ENABLE_ADDRSAN} MATCHES ON)
    target_compile_options(2006_VRPTW_Pareto PRIVATE -fsanitize=address)
    target_link_options(2006_VRPTW_Pareto PRIVATE -fsanitize=address)
endif ()

if (${ENABLE_UBSAN} MATCHES ON)
    target_compile_options(2006_VRPTW_Pareto PRIVATE -fsanitize=undefined)
    target_link_options(2006_VRPTW_Pareto PRIVATE -fsanitize=undefined)
endif ()

if (${ENABLE_TSAN} MATCHES ON)
    target_compile_options(2006_VRPTW_Pareto PRIVATE -fsanitize=thread)
    target_link_options(2006_VRPTW_Pareto PRIVATE -fsanitize=thread)
endif ()
