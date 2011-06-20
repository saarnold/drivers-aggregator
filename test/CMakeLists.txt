find_package( Boost COMPONENTS unit_test_framework )
if (Boost_FOUND)
    message(STATUS "boost/unit found, compiling unit test")
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)

    ADD_EXECUTABLE(timestamper-test test_timestamper.cpp)
    TARGET_LINK_LIBRARIES(timestamper-test sample_aggregator ${Boost_LIBRARIES})

    ADD_EXECUTABLE(streamaligner-test test_streamaligner.cpp)
    TARGET_LINK_LIBRARIES(streamaligner-test sample_aggregator ${Boost_LIBRARIES})

    ADD_EXECUTABLE(tstimestamper-test test_tstimestamper.cpp)
    TARGET_LINK_LIBRARIES(tstimestamper-test sample_aggregator ${Boost_LIBRARIES})
else(Boost_FOUND)
    message(STATUS "boost/unit not found, disabled unit test")
endif (Boost_FOUND)


