rock_testsuite(timestamper-test test_timestamper.cpp
    DEPS aggregator
    DEPS_PKGCONFIG base-types)
rock_testsuite(tstimestamper-test test_tstimestamper.cpp
    DEPS aggregator
    DEPS_PKGCONFIG base-types)
rock_testsuite(streamaligner-test test_streamaligner.cpp
    DEPS aggregator
    DEPS_PKGCONFIG base-types)

