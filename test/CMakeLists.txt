rock_testsuite(timestamper-test test_timestamper.cpp
    DEPS sample_aggregator
    DEPS_PKGCONFIG base-types)
rock_testsuite(tstimestamper-test test_tstimestamper.cpp
    DEPS sample_aggregator
    DEPS_PKGCONFIG base-types)
rock_testsuite(streamaligner-test test_streamaligner.cpp
    DEPS sample_aggregator
    DEPS_PKGCONFIG base-types)

