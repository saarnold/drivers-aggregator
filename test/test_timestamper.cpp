#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_timestamper"
#define BOOST_AUTO_TEST_MAIN

// #define PLOT_DEBUG

#include <iostream>
#include <numeric>

#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>  

#include <aggregator/TimestampEstimator.hpp>
#include <fstream>

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

    BOOST_REQUIRE_CLOSE(step.toSeconds(), estimator.getPeriod().toSeconds(), 1e-6);
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

    BOOST_REQUIRE_SMALL(mean,     error_limit);
    BOOST_REQUIRE_SMALL(variance, error_limit);
}

/**
 * helper class for unit testing
 * This class calculates the sample time,
 * hardware time and real time for a given sample number.
 * 
 * It also provides some functionallity to create plottable data
 * and performs some standard unit testing.
 * */
class Tester
{
private:
    std::ofstream debugFile;
public:
    //static latency of the sample
    base::Time sampleLatency;
    //maximum of random noise added ontop of the static latency
    base::Time sampleLatencyMaxNoise;

    //maximum noise of hardware timestamp
    base::Time hwTimeMaxNoise;

    //time of the first sample
    base::Time baseTime;
    
    //real period
    base::Time realPeriod;

    //the time the sampleTime is off every period
    base::Time periodDrift;

    //output, calculated by classe
    base::Time sampleTime;
    //output, calculated by classe
    base::Time hwTime;
    //output, calculated by classe
    base::Time realTime;
    
public:
    Tester(std::string debugFileName = "")
    {
	//init random nummer generator
	srand48(time(NULL));

	baseTime = base::Time::now();
	
#ifndef PLOT_DEBUG
	debugFileName = "";
#endif
	
	if(!debugFileName.empty())
	    debugFile.open(debugFileName.c_str(), std::ios::out);
	
	if(debugFile.is_open())
	    debugFile << "#diff times : hwTime, sampleTime, EstimatedTime" << std::endl;
    }

    void calculateSamples(int nr)
    {
	base::Time sampleLatencyNoise = base::Time::fromSeconds(drand48() * sampleLatencyMaxNoise.toSeconds());

	base::Time hwTimeNoise(base::Time::fromSeconds(drand48() * hwTimeMaxNoise.toSeconds()));

	sampleTime = baseTime + realPeriod * nr + periodDrift * nr + sampleLatency + sampleLatencyNoise;
	
	hwTime = baseTime + realPeriod * nr + hwTimeNoise;

	realTime = baseTime + realPeriod * nr;
    }
    
    void checkResult(base::Time estimatedTime)
    {
	if(debugFile.is_open())
        {
	    debugFile << (hwTime - realTime).toSeconds() << " " << (sampleTime - realTime).toSeconds() << " " << (estimatedTime - realTime).toSeconds() << std::endl; 
        }
	else{
	    //the estimate has to stay in the period
	    BOOST_REQUIRE_SMALL((estimatedTime - realTime).toSeconds(), realPeriod.toSeconds() / 10);
	}
    }
};

void test_timestamper_impl(int hardware_order, bool has_initial_period, bool has_drift, int init, double lost_rate)
{
    //this test case tries to emulate the values of an hokuyo laser scanner
    static const int COUNT = 10000;

    std::stringstream csv_filename;
    csv_filename << "test_timestamper";
    if (hardware_order < 0)
        csv_filename << "__hw_before";
    else if (hardware_order > 0)
        csv_filename << "__hw_after";
    if (has_initial_period)
        csv_filename << "__initial_period";
    if (has_drift)
        csv_filename << "__period_drift";
    if (init)
        csv_filename << "__init" << init;
    if (lost_rate)
        csv_filename << "__loss";

    Tester data(csv_filename.str());
    data.realPeriod = base::Time::fromSeconds(0.025);
    
    if (hardware_order != 0)
        data.sampleLatency = base::Time::fromSeconds(0.02);
    if (has_drift)
        data.periodDrift = base::Time::fromSeconds(0.0001);
    data.sampleLatencyMaxNoise = base::Time::fromSeconds(0.005);
    data.hwTimeMaxNoise = base::Time::fromMicroseconds(50);

    base::Time initial_period;
    if (has_initial_period)
        initial_period = base::Time::fromSeconds(0.025);
    
    //estimator for testing
    TimestampEstimator estimator(base::Time::fromSeconds(20),
	initial_period);
    
    for (int i = 0; i < COUNT; ++i)
    {
	data.calculateSamples(i);

        if (hardware_order < 0)
            estimator.updateReference(data.hwTime);

        if (drand48() < lost_rate)
            continue;

	base::Time estimatedTime = estimator.update(data.sampleTime);

        if (hardware_order > 0)
            estimator.updateReference(data.hwTime);

        if (i < init)
            continue;
	data.checkResult(estimatedTime);
    }
}

BOOST_AUTO_TEST_CASE(test_timestamper__plain)
{ test_timestamper_impl(0, false, false, 1000, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period)
{ test_timestamper_impl(-1, true, false, 0, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before)
{ test_timestamper_impl(-1, false, false, 1000, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period)
{ test_timestamper_impl(1, true, false, 0, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after)
{ test_timestamper_impl(1, false, false, 1000, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__initial_period)
{ test_timestamper_impl(0, true, false, 0, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__drift)
{ test_timestamper_impl(-1, true, true, 0, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__drift)
{ test_timestamper_impl(-1, false, true, 1000, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__drift)
{ test_timestamper_impl(1, true, true, 0, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__drift)
{ test_timestamper_impl(1, false, true, 1000, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__drift)
{ test_timestamper_impl(0, true, true, 0, 0); }
BOOST_AUTO_TEST_CASE(test_timestamper__drift)
{ test_timestamper_impl(0, false, true, 1000, 0); }

BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__loss)
{ test_timestamper_impl(-1, true, false, 0, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__loss)
{ test_timestamper_impl(-1, false, false, 1000, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__loss)
{ test_timestamper_impl(1, true, false, 0, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__loss)
{ test_timestamper_impl(1, false, false, 1000, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__loss)
{ test_timestamper_impl(0, true, false, 0, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__loss)
{ test_timestamper_impl(0, false, false, 1000, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__drift__loss)
{ test_timestamper_impl(-1, true, true, 0, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__drift__loss)
{ test_timestamper_impl(-1, false, true, 1000, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__drift__loss)
{ test_timestamper_impl(1, true, true, 0, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__drift__loss)
{ test_timestamper_impl(1, false, true, 1000, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__drift__loss)
{ test_timestamper_impl(0, true, true, 0, 0.01); }
BOOST_AUTO_TEST_CASE(test_timestamper__drift__loss)
{ test_timestamper_impl(0, false, true, 1000, 0.01); }

