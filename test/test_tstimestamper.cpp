#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE "test_timestamper"
#define BOOST_AUTO_TEST_MAIN

#include <iostream>
#include <numeric>

#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>

#include <aggregator/Timestamper.hpp>
using namespace aggregator;

BOOST_AUTO_TEST_CASE(test_simple_api)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);

    Timestamper<int> ts(base::Time::fromSeconds(1),
				  base::Time::fromSeconds(0),
				  base::Time::fromSeconds(0.1),
				  base::Time::fromSeconds(20)
				  );

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
	time = time + step;
	base::Time ref = time-base::Time::fromSeconds(0.05);
	ts.pushReference(ref);
	ts.pushItem(i,time);
	int j;
	base::Time t;
	BOOST_REQUIRE(ts.fetchItem(j,t,time));
	BOOST_CHECK_EQUAL(i,j);
	BOOST_CHECK_SMALL(t.toSeconds()-ref.toSeconds(),2e-6);
    }
}

BOOST_AUTO_TEST_CASE(test_getTimeFor)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);

    Timestamper<int> ts(base::Time::fromSeconds(1),
				  base::Time::fromSeconds(0),
				  base::Time::fromSeconds(0.1),
				  base::Time::fromSeconds(20)
				  );

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
	time = time + step;
	base::Time ref = time-base::Time::fromSeconds(0.05);
	ts.pushReference(ref);
	base::Time t;
	BOOST_REQUIRE(ts.getTimeFor(t));
	BOOST_CHECK_SMALL(t.toSeconds()-ref.toSeconds(),2e-6);
    }
}

BOOST_AUTO_TEST_CASE(test_spareItems)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);

    Timestamper<int> ts(base::Time::fromSeconds(1),
				  base::Time::fromSeconds(0),
				  base::Time::fromSeconds(0.1),
				  base::Time::fromSeconds(20)
				  );

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
	time = time + step;
	base::Time ref = time-base::Time::fromSeconds(0.05);
	ts.pushReference(ref);
	Timestamper<int>::ItemIterator it;
	it = ts.getSpareItem();
	ts.putSpareItem(it);
	it = ts.getSpareItem();
	it->item = i;
	it->time = time;
	ts.pushItem(it);
	int j;
	base::Time t;
	BOOST_REQUIRE(ts.fetchItem(j,t,time));
	BOOST_CHECK_EQUAL(i,j);
	BOOST_CHECK_SMALL(t.toSeconds()-ref.toSeconds(),2e-6);
    }
}

BOOST_AUTO_TEST_CASE(test_alt_fetch_api)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);

    Timestamper<int> ts(base::Time::fromSeconds(1),
				  base::Time::fromSeconds(0),
				  base::Time::fromSeconds(0.1),
				  base::Time::fromSeconds(20)
				  );

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
	time = time + step;
	base::Time ref = time-base::Time::fromSeconds(0.05);
	ts.pushReference(ref);
	ts.pushItem(i,time);
	int j;
	base::Time t;

	BOOST_REQUIRE(ts.itemAvailable(time));
	j = ts.item().item;
	t = ts.item().time;
	ts.popItem();
	BOOST_CHECK_EQUAL(i,j);
	BOOST_CHECK_SMALL(t.toSeconds()-ref.toSeconds(),2e-6);
    }
}


BOOST_AUTO_TEST_CASE(test_latency1)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);

    Timestamper<int> ts(base::Time::fromSeconds(1),
				  base::Time::fromSeconds(0),
				  base::Time::fromSeconds(0.1),
				  base::Time::fromSeconds(20)
				  );

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
	time = time + step;
	base::Time ref = time-base::Time::fromSeconds(0.05);
	ts.pushReference(ref);
	if (i >= 10) {
	    ts.pushItem(i-10,time-base::Time::fromSeconds(10*step.toSeconds()));
	    int j;
	    base::Time t;
	    BOOST_REQUIRE(ts.fetchItem(j,t,time));
	    BOOST_CHECK_EQUAL(i-10,j);
	    BOOST_CHECK_SMALL(t.toSeconds()-
			      (ref.toSeconds()-10*step.toSeconds()),2e-6);
	}
    }
}

BOOST_AUTO_TEST_CASE(test_latency2)
{
    base::Time time = base::Time::now();
    base::Time step = base::Time::fromSeconds(0.1);

    Timestamper<int> ts(base::Time::fromSeconds(1),
				  base::Time::fromSeconds(0),
				  base::Time::fromSeconds(0.1),
				  base::Time::fromSeconds(20)
				  );

    TimestampEstimator estimator(base::Time::fromSeconds(2), 0);
    for (int i = 0; i < 10000; ++i)
    {
	time = time + step;
	base::Time ref = time-base::Time::fromSeconds(0.05);
	ts.pushItem(i,time);
	if (i >= 10) {
	    ts.pushReference(ref-base::Time::fromSeconds(10*step.toSeconds()));
	    int j;
	    base::Time t;
	    BOOST_REQUIRE(ts.fetchItem(j,t,time));
	    BOOST_CHECK_EQUAL(i-10,j);
	    BOOST_CHECK_SMALL(t.toSeconds()-
			      (ref.toSeconds()-10*step.toSeconds()),2e-6);
	}
    }
}

