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

string lastSample;

void test_callback( const base::Time &time, const string& sample )
{
    lastSample = sample;
}

BOOST_AUTO_TEST_CASE( order_test )
{
    SampleReader reader; 
    reader.setTimeout( base::Time(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 4, base::Time(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 4, base::Time(0,0) ); 

    reader.push( s1, base::Time(0.0), string("a") ); 
    reader.push( s1, base::Time(2.0), string("c") ); 
    reader.push( s2, base::Time(1.0), string("b") ); 

    reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
}

BOOST_AUTO_TEST_CASE( timeout_test )
{
    SampleReader reader; 
    reader.setTimeout( base::Time(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 4, base::Time(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 4, base::Time(0,0) ); 

    reader.push( s1, base::Time(0.0), string("a") ); 
    reader.push( s1, base::Time(1.0), string("b") ); 
    reader.push( s1, base::Time(0.0), string("1") ); 
    reader.push( s1, base::Time(3.0), string("d") ); 
    reader.push( s2, base::Time(0.0), string("2") ); 
    reader.push( s2, base::Time(2.0), string("c") ); 

    cout << reader;
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    cout << reader;
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    cout << reader;
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
    cout << reader;
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "d" );
    cout << reader;
}
