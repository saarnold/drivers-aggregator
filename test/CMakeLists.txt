find_package( Boost COMPONENTS unit_test_framework )
if (Boost_FOUND)
    message(STATUS "boost/unit found, compiling unit test")
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${CMAKE_SOURCE_DIR}/src)

    ADD_EXECUTABLE(timestamper-test test_timestamper.cpp)
    TARGET_LINK_LIBRARIES(timestamper-test sample_aggregator ${Boost_LIBRARIES})

    ADD_EXECUTABLE(samplereader-test test_samplereader.cpp)
    TARGET_LINK_LIBRARIES(samplereader-test sample_aggregator ${Boost_LIBRARIES})
else(Boost_FOUND)
    message(STATUS "boost/unit not found, disabled unit test")
endif (Boost_FOUND)


