add_library(sample_aggregator SHARED TimestampEstimator.cpp)
install(FILES TimestampEstimator.hpp StreamAligner.hpp PullStreamAligner.hpp StreamAlignerStatus.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS sample_aggregator
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
