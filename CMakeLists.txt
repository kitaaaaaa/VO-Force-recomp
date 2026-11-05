# force - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(force LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(FORCE_SOURCES
    src/main.cpp
    src/stl_compat.cpp
    src/debug_tools.cpp
)

if(WIN32)
    add_executable(force WIN32 ${FORCE_SOURCES})
else()
    add_executable(force ${FORCE_SOURCES})
endif()

rexglue_setup_target(force GPU_PLUGINS xenos)
