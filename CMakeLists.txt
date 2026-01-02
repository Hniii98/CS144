cmake_minimum_required(VERSION 3.24.2)

project(minnow CXX)

# Set -j flag automatic if use Makefiles generator
if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
  set(CMAKE_MAKE_PROGRAM "${PROJECT_SOURCE_DIR}/scripts/make-parallel.sh" CACHE STRING "" FORCE)
endif()

# Check if set binary director
if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Minnow must be built outside its source directory, e.g. `cmake -B build`.")
endif()

# Include sub configuration of cmake 
include(etc/build_type.cmake)
include(etc/cflags.cmake)
include(etc/scanners.cmake)
include(etc/tests.cmake)

# Add  searching path of header files
include_directories("${PROJECT_SOURCE_DIR}/util")
include_directories("${PROJECT_SOURCE_DIR}/src")

# Add sub modules of project
add_subdirectory("${PROJECT_SOURCE_DIR}/util")
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
add_subdirectory("${PROJECT_SOURCE_DIR}/apps")

# Export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
