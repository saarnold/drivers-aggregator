#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_samplereader"
#define BOOST_AUTO_TEST_MAIN

#include <iostream>
#include <numeric>

#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>  

#include "SampleReader.hpp"

using namespace aggregator;
using namespace std;

template <class T> void reading_callback(base::Time ts, T data)
{
    cout << "ts: " << ts.seconds << ":" << ts.microseconds << " data: " << data << endl;
}

BOOST_AUTO_TEST_CASE( aggregator_test )
{
    SampleReader aggr; 
    aggr.setTimeout( base::Time(2,0) );

    // callback, buffer_size, period_time
    int s1 = aggr.registerStream<int>( &reading_callback<int>, 4, base::Time(1,0) ); 
    int s2 = aggr.registerStream<string>( &reading_callback<string>, 2, base::Time(2,0) ); 
    int s3 = aggr.registerStream<double>( &reading_callback<double> ); 

    aggr.push( s1, base::Time(1), 1 );
    aggr.push( s2, base::Time(0), string("test1") );
    aggr.push( s2, base::Time(2), string("test2") );
    aggr.push( s2, base::Time(4), string("test3") );
    aggr.push( s1, base::Time(2), 2 );
    aggr.push( s1, base::Time(2), 3 );
    aggr.push( s1, base::Time(3), 4 );
    aggr.push( s1, base::Time(4), 5 );
    aggr.push( s1, base::Time(5), 6 );
    aggr.push( s3, base::Time(5), 1.55 );

    while( aggr.step() )
    {
	std::cout << aggr;
    }
}

