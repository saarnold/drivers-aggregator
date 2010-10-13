#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_samplereader"
#define BOOST_AUTO_TEST_MAIN

#include <iostream>
#include <numeric>

#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>  

#include "StreamAligner.hpp"

using namespace aggregator;
using namespace std;

string lastSample;

void test_callback( const base::Time &time, const string& sample )
{
    lastSample = sample;
}

BOOST_AUTO_TEST_CASE( order_test )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2,0) ); 

    reader.push( s1, base::Time::fromSeconds(0.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(2.0), string("c") ); 
    reader.push( s2, base::Time::fromSeconds(1.0), string("b") ); 

    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
}


BOOST_AUTO_TEST_CASE( drop_test )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 

    reader.push( s1, base::Time::fromSeconds(10.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(11.0), string("b") ); 
    reader.push( s1, base::Time::fromSeconds(10.0), string("3") ); 

    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
}

BOOST_AUTO_TEST_CASE( copy_state_test )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 

    reader.push( s1, base::Time::fromSeconds(10.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(11.0), string("b") ); 
    reader.push( s1, base::Time::fromSeconds(10.0), string("3") ); 

    StreamAligner reader2;
    reader2.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    reader2.copyState( reader );

    BOOST_CHECK_EQUAL( reader.getLatency().toSeconds(), reader2.getLatency().toSeconds() );

    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );

    lastSample = ""; reader2.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader2.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader2.step(); BOOST_CHECK_EQUAL( lastSample, "" );
}

BOOST_AUTO_TEST_CASE( timeout_test )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(0,0) ); 

    reader.push( s1, base::Time::fromSeconds(10.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(11.0), string("b") ); 
    
    // aligner should wait here since latency is < timeout
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );

    reader.push( s1, base::Time::fromSeconds(12.0), string("c") ); 

    // now only a and b should be available
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );

    // and b
    reader.push( s1, base::Time::fromSeconds(13.0), string("e") ); 
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );

    reader.push( s2, base::Time::fromSeconds(12.5), string("d") ); 

    // the sample on s2 should release everything in s1
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "d" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );

    // this is checking the lookahead
    reader.push( s2, base::Time::fromSeconds(14.0), string("f") ); 

    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "e" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "f" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
}
