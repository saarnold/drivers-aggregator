#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_timestamper"
#define BOOST_AUTO_TEST_MAIN

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

    // Drift of the period in s/s
    //
    // I.e. it is the rate of change of period in seconds per second
    base::Time periodDrift;

    //output, calculated by classe
    base::Time sampleTime;
    //output, calculated by classe
    base::Time hwTime;
    //output, calculated by classe
    base::Time realTime;
    // output: realPeriod with drift, calculated in update
    base::Time actualPeriod;
    
public:
    Tester(std::string debugFileName = "")
    {
	//init random nummer generator
	srand48(time(NULL));

	baseTime = base::Time::now();
	
	if(!debugFileName.empty())
	    debugFile.open(debugFileName.c_str(), std::ios::out);
	
	if(debugFile.is_open())
        {
            debugFile << std::setprecision(5) << std::endl;
	    debugFile << "# (hardware-real) (sample-real) (estimate-real) (estimated_period-actual_period) (est-last_est) (real-last_real) period" << std::endl;
        }

    }

    void calculateSamples(int nr)
    {
        base::Time sampleLatencyMaxNoise;
        if (nr > 0)
            sampleLatencyMaxNoise = this->sampleLatencyMaxNoise;
        else
            sampleLatencyMaxNoise = realPeriod * 0.09;
        base::Time sampleLatencyNoise = base::Time::fromSeconds(drand48() * sampleLatencyMaxNoise.toSeconds());

	base::Time hwTimeNoise(base::Time::fromSeconds(drand48() * hwTimeMaxNoise.toSeconds()));

        actualPeriod = realPeriod + periodDrift * nr;
	realTime   = baseTime + realPeriod * nr + periodDrift * nr * (nr + 1) / 2;
	sampleTime = realTime + sampleLatency + sampleLatencyNoise;
	hwTime     = realTime + hwTimeNoise;
    }

    void addResultToPlot(base::Time estimatedTime, base::Time estimatedPeriod)
    {
        static base::Time lastEstimatedTime = estimatedTime;
        static base::Time lastRealTime = realTime;

	if(debugFile.is_open())
        {
	    debugFile << (hwTime - realTime).toSeconds() / actualPeriod.toSeconds()
                << " " << (sampleTime - realTime - sampleLatency).toSeconds() / actualPeriod.toSeconds()
                << " " << (estimatedTime - realTime).toSeconds() / actualPeriod.toSeconds()
                << " " << (estimatedPeriod - actualPeriod).toSeconds() / actualPeriod.toSeconds()
                << " " << (estimatedTime - lastEstimatedTime).toSeconds()
                << " " << (realTime - lastRealTime).toSeconds()
                << " " << actualPeriod.toSeconds()
                << std::endl; 
        }
        lastEstimatedTime = estimatedTime;
        lastRealTime = realTime;
    }
    
    void checkResult(base::Time estimatedTime, base::Time estimatedPeriod)
    {
        addResultToPlot(estimatedTime, estimatedPeriod);
        BOOST_CHECK_SMALL((estimatedTime - realTime).toSeconds(), actualPeriod.toSeconds() / 10);
    }
};

enum LOSS_ANNOUNCE_METHODS
{
    USE_NONE,
    USE_UPDATE_LOSS,
    USE_INDEX
};
void test_timestamper_impl(int hardware_order, bool has_initial_period, bool has_drift, int init, double loss_rate, LOSS_ANNOUNCE_METHODS loss_announce_method = USE_NONE)
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
    if (loss_rate)
        csv_filename << "__loss";
    csv_filename << ".csv";

    Tester data(csv_filename.str());
    data.realPeriod = base::Time::fromSeconds(0.025);
    
    if (hardware_order != 0)
    {
        data.sampleLatency = base::Time::fromSeconds(0.02);
        if (init < 900)
            init = 900;
    }

    if (has_drift)
    {
        // experimental value. In these tests, the filter does not cope anymore
        // if the period drift is higher than that
        data.periodDrift = base::Time::fromSeconds(3e-5);
    }
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

        if (drand48() < loss_rate)
        {
            if (loss_announce_method == USE_UPDATE_LOSS)
                estimator.updateLoss();
            continue;
        }

        base::Time estimatedTime;
        if (loss_announce_method == USE_INDEX)
            estimatedTime = estimator.update(data.sampleTime, i);
        else
            estimatedTime = estimator.update(data.sampleTime);

        if (hardware_order > 0)
            estimator.updateReference(data.hwTime);

        base::Time period;
        if (estimator.haveEstimate())
            period = estimator.getPeriod();

        if (i < init)
            data.addResultToPlot(estimatedTime, period);
        else
            data.checkResult(estimatedTime, period);
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
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__drift)
// { test_timestamper_impl(-1, true, true, 0, 0); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__drift)
// { test_timestamper_impl(-1, false, true, 1000, 0); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__drift)
// { test_timestamper_impl(1, true, true, 0, 0); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__drift)
// { test_timestamper_impl(1, false, true, 1000, 0); }
// BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__drift)
// { test_timestamper_impl(0, true, true, 0, 0); }
// BOOST_AUTO_TEST_CASE(test_timestamper__drift)
// { test_timestamper_impl(0, false, true, 1000, 0); }

BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__loss_updateLoss)
{ test_timestamper_impl(-1, true, false, 0, 0.01, USE_UPDATE_LOSS); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__loss_updateLoss)
{ test_timestamper_impl(-1, false, false, 1000, 0.01, USE_UPDATE_LOSS); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__loss_updateLoss)
{ test_timestamper_impl(1, true, false, 0, 0.01, USE_UPDATE_LOSS); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__loss_updateLoss)
{ test_timestamper_impl(1, false, false, 1000, 0.01, USE_UPDATE_LOSS); }
BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__loss_updateLoss)
{ test_timestamper_impl(0, true, false, 0, 0.01, USE_UPDATE_LOSS); }
BOOST_AUTO_TEST_CASE(test_timestamper__loss_updateLoss)
{ test_timestamper_impl(0, false, false, 1000, 0.01, USE_UPDATE_LOSS); }

BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__loss_index)
{ test_timestamper_impl(-1, true, false, 0, 0.01, USE_INDEX); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__loss_index)
{ test_timestamper_impl(-1, false, false, 1000, 0.01, USE_INDEX); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__loss_index)
{ test_timestamper_impl(1, true, false, 0, 0.01, USE_INDEX); }
BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__loss_index)
{ test_timestamper_impl(1, false, false, 1000, 0.01, USE_INDEX); }
BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__loss_index)
{ test_timestamper_impl(0, true, false, 0, 0.01, USE_INDEX); }
BOOST_AUTO_TEST_CASE(test_timestamper__loss_index)
{ test_timestamper_impl(0, false, false, 1000, 0.01, USE_INDEX); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__initial_period__drift__loss)
// { test_timestamper_impl(-1, true, true, 0, 0.01); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_before__drift__loss)
// { test_timestamper_impl(-1, false, true, 1000, 0.01); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__initial_period__drift__loss)
// { test_timestamper_impl(1, true, true, 0, 0.01); }
// BOOST_AUTO_TEST_CASE(test_timestamper__hw_after__drift__loss)
// { test_timestamper_impl(1, false, true, 1000, 0.01); }
// BOOST_AUTO_TEST_CASE(test_timestamper__initial_period__drift__loss)
// { test_timestamper_impl(0, true, true, 0, 0.01); }
// BOOST_AUTO_TEST_CASE(test_timestamper__drift__loss)
// { test_timestamper_impl(0, false, true, 1000, 0.01); }

