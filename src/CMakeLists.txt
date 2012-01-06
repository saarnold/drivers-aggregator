rock_library(aggregator
    SOURCES TimestampEstimator.cpp StreamAlignerStatus.cpp
    DEPS_PKGCONFIG base-types base-lib
    HEADERS TimestampEstimator.hpp TimestampEstimatorStatus.hpp
    StreamAligner.hpp PullStreamAligner.hpp StreamAlignerStatus.hpp)
    
