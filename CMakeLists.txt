cmake_minimum_required(VERSION 2.6)
include(Rock)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

rock_init(aggregator 0.1)
rock_standard_layout()

CONFIGURE_FILE(aggregator.pc.in aggregator.pc @ONLY)
INSTALL(FILES ${CMAKE_BINARY_DIR}/aggregator.pc DESTINATION lib/pkgconfig)

