#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_timestamper"
#define BOOST_AUTO_TEST_MAIN

#include <iostream>
#include <numeric>

#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>  

#include "TimestampEstimator.hpp"
using namespace aggregator;

BOOST_AUTO_TEST_CASE(test_perfect_stream)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.01);

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
        time = time + step;
        BOOST_REQUIRE_CLOSE(time.toSeconds(), estimator.update(time).toSeconds(), 0.0000001);
        BOOST_REQUIRE_EQUAL(0, estimator.getLostSampleCount());
    }

    BOOST_REQUIRE_CLOSE(step.toSeconds(), estimator.getPeriod().toSeconds(), 0.0000001);
}

BOOST_AUTO_TEST_CASE(test_noisy_latency)
{
    srand48(time(NULL));
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.01);

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    // First feed 10 samples, then check
    for (int i = 0; i < 10000; ++i)
    {
        time = time + step;
        base::Time noisy = time + base::Time::fromSeconds(drand48() * 0.001);
        base::Time estimate = estimator.update(noisy);
        BOOST_REQUIRE_EQUAL(0, estimator.getLostSampleCount());

        if (i > 10)
            BOOST_REQUIRE_CLOSE(time.toSeconds(), estimate.toSeconds(), 0.1);
    }

    BOOST_REQUIRE_CLOSE(step.toSeconds(), estimator.getPeriod().toSeconds(), 0.1);
}

BOOST_AUTO_TEST_CASE(test_drift_and_noisy)
{
    srand48(time(NULL));
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);
    base::Time drift = base::Time::fromSeconds(1e-6);

    TimestampEstimator estimator(base::Time::fromSeconds(1), 2);

    // First feed 10 samples, then check
    for (int i = 0; i < 10000; ++i)
    {
        step = step + drift;
        time = time + step;
        base::Time estimate = estimator.update(time + base::Time::fromSeconds(drand48() * 1e-6));
        BOOST_REQUIRE_EQUAL(0, estimator.getLostSampleCount());

        //std::cerr << i << " period error: "     << (estimator.getPeriod().toSeconds() - step.toSeconds()) << std::endl;
        //std::cerr << i << " estimation error: " << (estimate - time).toSeconds() << std::endl;
        if (i > 10)
            BOOST_REQUIRE_CLOSE(time.toSeconds(), estimate.toSeconds(), 0.1);
    }

    BOOST_REQUIRE_CLOSE(step.toSeconds(), estimator.getPeriod().toSeconds(), 0.3);
}

BOOST_AUTO_TEST_CASE(test_drift_noisy_lost_samples)
{
    srand48(time(NULL));
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);
    base::Time drift = base::Time::fromSeconds(1e-5);

    static const int COUNT = 10000;
    static const double NOISE = 1e-2;

    TimestampEstimator estimator(base::Time::fromSeconds(5), 0);

    int total_skip = 0;
    std::vector<double> deltas;
    // First feed 10 samples, and then start checking
    for (int i = 0; i < COUNT; ++i)
    {
        step = step + drift;
        time = time + step;
        if (drand48() > 0.1)
        {
            base::Time estimate = estimator.update(time + base::Time::fromSeconds(drand48() * NOISE));
            double delta = time.toSeconds() - estimate.toSeconds();
            deltas.push_back(delta);
        }
        else
            total_skip++;
    }

    double mean = std::accumulate(deltas.begin(), deltas.end(), 0) / deltas.size();
    double variance = 0;
    for (std::vector<double>::const_iterator it = deltas.begin(); it != deltas.end(); ++it)
        variance += (*it - mean) * (*it - mean);
    variance = sqrt(variance / COUNT);

    static const double error_limit = NOISE + drift.toSeconds() * 50;

    std::cerr << "acceptable error: " << error_limit << " for period of " << step.toSeconds() << " and drift of " << drift.toSeconds() << std::endl;
    std::cerr << "losing " << total_skip << " samples" << std::endl;
    std::cerr << "error mean: " << mean << std::endl;
    std::cerr << "error variance: " << variance << std::endl;
    BOOST_REQUIRE_SMALL(mean,     error_limit);
    BOOST_REQUIRE_SMALL(variance, error_limit);
}

