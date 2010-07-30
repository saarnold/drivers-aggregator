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
    int s2 = reader.registerStream<string>( &test_callback, 4, base::Time(2,0) ); 

    reader.push( s1, base::Time(0.0), string("a") ); 
    reader.push( s1, base::Time(2.0), string("c") ); 
    reader.push( s2, base::Time(1.0), string("b") ); 

    lastSample = "";
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
}

BOOST_AUTO_TEST_CASE( timeout_test )
{
    SampleReader reader; 
    reader.setTimeout( base::Time(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 5, base::Time(0,0) ); 

    reader.push( s1, base::Time(10.0), string("a") ); 
    reader.push( s1, base::Time(11.0), string("b") ); 
    reader.push( s1, base::Time(10.0), string("3") ); 
    reader.push( s1, base::Time(13.0), string("d") ); 
    reader.push( s2, base::Time(12.0), string("c") ); 
    reader.push( s1, base::Time(16.0), string("e") ); 
    reader.push( s2, base::Time(13.0), string("4") ); 
    reader.push( s1, base::Time(17.0), string("f") ); 

    lastSample = "";
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "d" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "e" );
    reader.step(); BOOST_CHECK_EQUAL( lastSample, "f" );
}
