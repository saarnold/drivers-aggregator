add_library(sample_aggregator SHARED TimestampEstimator.cpp StreamAlignerStatus.cpp)
install(FILES TimestampSynchronizer.hpp TimestampEstimator.hpp StreamAligner.hpp PullStreamAligner.hpp StreamAlignerStatus.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS sample_aggregator
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
