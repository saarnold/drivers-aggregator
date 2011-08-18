rock_library(aggregator
    SOURCES TimestampEstimator.cpp StreamAlignerStatus.cpp
    DEPS_PKGCONFIG base-types
    HEADERS Timestamper.hpp TimestampEstimator.hpp TimestampEstimatorStatus.hpp
    StreamAligner.hpp PullStreamAligner.hpp StreamAlignerStatus.hpp)
    
