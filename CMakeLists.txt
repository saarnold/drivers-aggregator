cmake_minimum_required(VERSION 2.6)
find_package(Rock)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

rock_init(aggregator 0.1)
rock_standard_layout()

