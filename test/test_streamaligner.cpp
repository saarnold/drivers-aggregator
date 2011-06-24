#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_samplereader"
#define BOOST_AUTO_TEST_MAIN

#include <iostream>
#include <numeric>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>  

#include "StreamAligner.hpp"
#include "PullStreamAligner.hpp"

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

    // callback, buffer_size, period_time, (optional) priority
    int s1 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2) ); 
    int s2 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2), 1 );

    reader.push( s1, base::Time::fromSeconds(1.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(3.0), string("c") ); 
    reader.push( s2, base::Time::fromSeconds(2.0), string("b") ); 
    reader.push( s2, base::Time::fromSeconds(3.0), string("d") ); 
    reader.push( s2, base::Time::fromSeconds(4.0), string("f") ); 
    reader.push( s1, base::Time::fromSeconds(4.0), string("e") ); 

    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "d" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "e" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "f" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
}


BOOST_AUTO_TEST_CASE( remove_stream )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time, (optional) priority
    int s1 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2) ); 
    int s3 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2) ); 
    int s2 = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2), 1 );

    reader.push( s1, base::Time::fromSeconds(1.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(3.0), string("c") ); 
    reader.push( s2, base::Time::fromSeconds(2.0), string("b") ); 
    reader.push( s2, base::Time::fromSeconds(3.0), string("d") ); 
    reader.push( s2, base::Time::fromSeconds(4.0), string("f") ); 
    reader.push( s1, base::Time::fromSeconds(4.0), string("e") ); 

    reader.unregisterStream(s3);
    std::cout << reader.getStatus() << std::endl;
    
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "d" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "e" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "f" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
    
    int s3_new = reader.registerStream<string>( &test_callback, 4, base::Time::fromSeconds(2) ); 
    BOOST_CHECK_EQUAL(s3, s3_new);
}

BOOST_AUTO_TEST_CASE( drop_test )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 

    reader.push( s1, base::Time::fromSeconds(10.0), string("a") ); 
    reader.push( s1, base::Time::fromSeconds(11.0), string("b") ); 
    bool gotException = false;
    try {
    reader.push( s1, base::Time::fromSeconds(10.0), string("3") ); 
    } catch (std::runtime_error &e)
    {
	gotException = true;
    } 
    BOOST_CHECK_EQUAL(gotException, true);
    
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
    bool gotException = false;
    try {
    reader.push( s1, base::Time::fromSeconds(10.0), string("3") ); 
    } catch (std::runtime_error &e)
    {
	gotException = true;
    } 
    BOOST_CHECK_EQUAL(gotException, true);

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

BOOST_AUTO_TEST_CASE( data_on_same_time_zero_lookahead )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 5, base::Time() ); 
    
    reader.push( s1, base::Time::fromSeconds(2.0), string("a") ); 
    reader.push( s2, base::Time::fromSeconds(2.0), string("b") ); 

    lastSample = ""; 
    reader.step(); 
    BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; 
    reader.step();
    BOOST_CHECK_EQUAL( lastSample, "b" );
}

BOOST_AUTO_TEST_CASE( data_on_same_time_zero_lookahead_advanced )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 5, base::Time() ); 
    int s3 = reader.registerStream<string>( &test_callback, 5, base::Time() ); 
    int s4 = reader.registerStream<string>( &test_callback, 5, base::Time() ); 
    
    reader.push( s4, base::Time::fromSeconds(2.0), string("d") ); 
    reader.push( s3, base::Time::fromSeconds(2.0), string("c") ); 
    reader.push( s1, base::Time::fromSeconds(2.0), string("a") ); 
    reader.push( s2, base::Time::fromSeconds(2.0), string("b") ); 

    lastSample = ""; 
    reader.step(); 
    BOOST_CHECK( lastSample != "" );

    lastSample = ""; 
    reader.step(); 
    BOOST_CHECK( lastSample != "" );

    lastSample = ""; 
    reader.step(); 
    BOOST_CHECK( lastSample != "" );

    lastSample = ""; 
    reader.step(); 
    BOOST_CHECK( lastSample != "" );
}


BOOST_AUTO_TEST_CASE( get_status )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(0,0) ); 

    std::cout << reader.getStatus();
}

/**
 * This testcase checks if data is replayed, if there is 
 * only data on one stream available
 * */
BOOST_AUTO_TEST_CASE( data_on_one_stream_test )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(0,0) ); 

    
    reader.push( s2, base::Time::fromSeconds(1.0), string("a") ); 

    //instant replay, as perios of s2 is zero
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
}

/**
 * This testcase checks, if all samples are replayed,
 * if a newer sample is given to the aligner first.  
 * This test case is about the inital case.
 * */
BOOST_AUTO_TEST_CASE( newer_data_first_init_case)
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(2,0) ); 
    int s2 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(0,0) ); 

    reader.push( s1, base::Time::fromSeconds(1.1), string("b") ); 
    
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
    
    reader.push( s2, base::Time::fromSeconds(1.0), string("a") ); 
    
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
}

/**
 * This test case check weather the aligner waits 
 * the full timeout again after he replayed a sample
 * from a stream
 * */
BOOST_AUTO_TEST_CASE( advanced_timout )
{
    StreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    // callback, buffer_size, period_time
    int s1 = reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(1,0) ); 
    reader.registerStream<string>( &test_callback, 5, base::Time::fromSeconds(0,0) ); 

    reader.push( s1, base::Time::fromSeconds(1.0), string("a") ); 
    
    reader.push( s1, base::Time::fromSeconds(1.1), string("b") ); 
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
    
    reader.push( s1, base::Time::fromSeconds(3.1), string("c") ); 
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "c" );

    //bigger than period, bus smaller than timeout, do not replay
    reader.push( s1, base::Time::fromSeconds(4.2), string("d") ); 
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );
}

template <class T>
struct pull_object
{
    pull_object() : hasNext( false ) {}

    void setNext(base::Time ts, const T& next)
    {
	next_ts = ts;
	next_value = next;
	hasNext = true;
    }

    bool getNext(base::Time& ts, T& next)
    {
	if( hasNext )
	{
	    next = next_value;
	    ts = next_ts;
	    hasNext = false;
	    return true;
	}
	return false;
    }

    bool hasNext;
    T next_value;
    base::Time next_ts;
};


BOOST_AUTO_TEST_CASE( pull_stream_test )
{
    PullStreamAligner reader; 
    reader.setTimeout( base::Time::fromSeconds(2.0) );

    pull_object<string> p1;
    pull_object<string> p2;

    // callback, buffer_size, period_time, (optional) priority
    reader.registerStream<string>( boost::bind( &pull_object<string>::getNext, &p1, _1, _2 ), &test_callback, 4, base::Time::fromSeconds(2) ); 
    reader.registerStream<string>( boost::bind( &pull_object<string>::getNext, &p2, _1, _2 ), &test_callback, 4, base::Time::fromSeconds(2), 1 );

    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "" );

    p1.setNext( base::Time::fromSeconds(2.0), string("b") ); 
    p2.setNext( base::Time::fromSeconds(1.0), string("a") ); 
    while( reader.pull() );
    
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "a" );
    lastSample = ""; reader.step(); BOOST_CHECK_EQUAL( lastSample, "b" );
}

