project(pose_estimator)
set(PROJECT_VERSION 1.0)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

add_definitions(-Wall)

pkg_check_modules(DFKI REQUIRED "base-types")
include_directories(${DFKI_INCLUDE_DIRS})
link_directories(${DFKI_LIBRARY_DIRS})

add_subdirectory(src)
add_subdirectory(test)


