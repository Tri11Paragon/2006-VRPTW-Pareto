cmake_minimum_required(VERSION 3.24)
project(2006_VRPTW_Pareto)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)
option(BUILD_GUI "Build the GUI component" ON)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(libraries/BLT)
if(${BUILD_GUI})
    add_subdirectory(libraries/glfw-3.3.8)

    find_package(OpenGL REQUIRED)
endif ()

file(GLOB_RECURSE VRPTW_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB IMGUI_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/*.cpp")
file(GLOB IMPLOT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/implot/*.cpp")
set(IMGUI_BACKEND_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/backends/imgui_impl_glfw.cpp")
list(APPEND IMGUI_BACKEND_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/backends/imgui_impl_opengl3.cpp")

include_directories(include/)

if(${BUILD_GUI})
	include_directories(libraries/imgui)
	include_directories(libraries/implot)
	include_directories(libraries/imgui/backends)
endif()

if(${BUILD_GUI})
	add_executable(2006_VRPTW_Pareto ${VRPTW_BUILD_FILES} ${IMGUI_BUILD_FILES} ${IMGUI_BACKEND_BUILD_FILES} ${IMPLOT_BUILD_FILES})
else()
	add_executable(2006_VRPTW_Pareto ${VRPTW_BUILD_FILES})
endif()


target_link_libraries(2006_VRPTW_Pareto BLT)

target_compile_options(2006_VRPTW_Pareto PRIVATE -Wall -Werror -Wpedantic -Wno-comment)
target_link_options(2006_VRPTW_Pareto PRIVATE -Wall -Werror -Wpedantic -Wno-comment)

if (${BUILD_GUI})
    target_link_libraries(2006_VRPTW_Pareto glfw)
    target_link_libraries(2006_VRPTW_Pareto OpenGL::GL)
endif ()

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
