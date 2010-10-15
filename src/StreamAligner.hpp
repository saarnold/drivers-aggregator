#ifndef __AGGREGATOR_HPP__
#define __AGGREGATOR_HPP__

#include <base/time.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <stdexcept> 
#include <iostream>

namespace aggregator {

    class StreamAligner
    {
	class StreamBase
	{
	    public:
		StreamBase() {}
		virtual base::Time pop() = 0;
		virtual bool hasData() const = 0;
		virtual int getPriority() const = 0;
		virtual base::Time nextTimeStamp() const = 0;
		virtual std::pair<size_t, size_t> getBufferStatus() const = 0;
		virtual void copyState( const StreamBase& other ) = 0;

		friend std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner::StreamBase &base);
	};

	template <class T> class Stream : public StreamBase
	{
	    typedef std::pair<base::Time,T> item;
	    std::queue<item> buffer;
	    size_t bufferSize;
	    boost::function<void (base::Time ts, T value)> callback;
	    base::Time period; 
	    base::Time lastTime;
	    int priority;

	public:
	    Stream( boost::function<void (base::Time ts, T value)> callback, size_t bufferSize, base::Time period, int priority )
		: bufferSize( bufferSize ), callback(callback), period(period), priority(priority) {}

	    virtual int getPriority() const
	    {
		return priority;
	    }

	    virtual std::pair<size_t, size_t> getBufferStatus() const
	    {
		return std::make_pair( buffer.size(), bufferSize );
	    }

	    virtual void copyState( const StreamBase& other )
	    {
		const Stream<T> &stream(static_cast<const Stream<T>& >(other));

		lastTime = stream.lastTime;
		buffer = stream.buffer;
		bufferSize = stream.bufferSize;
	    }

	    void push( base::Time ts, T data ) 
	    { 
		if( ts < lastTime )
		    return;

		lastTime = ts;

		// if the buffer is full, make some space
		// sorry old data, you gotta go!
		while( bufferSize > 0 && buffer.size() >= bufferSize )
		{
		    buffer.pop();
		}

		buffer.push( std::make_pair(ts, data) ); 
	    }

	    /** take the last item of the stream queue and 
	     * call the callback 
	     */
	    base::Time pop() 
	    { 
		if( hasData() )
		{
		    base::Time ts = buffer.front().first;
		    callback( ts, buffer.front().second );
		    buffer.pop();
		    return ts;
		}

		throw std::runtime_error("pop() called on stream with no data.");
	    }

	    bool hasData() const
	    { return !buffer.empty(); }

	    base::Time nextTimeStamp() const
	    {
		if( hasData() )
		    return buffer.front().first;
		else 
		    return lastTime + period;
	    }
	};

	static bool compareStreams( const StreamBase* b1, const StreamBase* b2 )
	{
	    const base::Time &ts1( b1->nextTimeStamp() );
	    const base::Time &ts2( b2->nextTimeStamp() );

	    return ts1 < ts2 || (ts1 == ts2 && b1->getPriority() < b2->getPriority());
	}

	typedef std::vector<StreamBase*> stream_vector;
	stream_vector streams;
	base::Time timeout;
	bool initialized;

	/** time of the last sample that came in */
	base::Time latest_ts;

	/** time of the last sample that went out */
	base::Time current_ts;

	double buffer_size_factor;

    public:
	explicit StreamAligner(base::Time timeout = base::Time::fromSeconds(1))
	    : timeout(timeout), initialized(false), buffer_size_factor(2.0) {}

	~StreamAligner()
	{
	    for(stream_vector::iterator it=streams.begin();it != streams.end();it++)
		delete *it;
	}

	/** will take the state of other StreamAligner and make it the state of this 
	 * object. State constitutes current_time and latest_time as well as all the stream
	 * content, but not the configuration.
	 */
	void copyState(const StreamAligner& other)
	{
	    initialized = other.initialized;
	    latest_ts = other.latest_ts;
	    current_ts = other.current_ts;

	    assert( streams.size() == other.streams.size() );
	    for(size_t i=0;i<streams.size();i++)
	    {
		streams[i]->copyState( *other.streams[i] );
	    }
	}

	/** Set the time the Estimator will wait for an expected reading on any of the streams.
	 * This number effectively puts an upper limit to the lag that can be created due to 
	 * delay or missing values on the channels.
	 */
	void setTimeout( base::Time t )
	{
	    timeout = t;
	}

	/** Will register a stream with the aggregator.
	 *
	 * @param callback - will be called for data gone through the synchronization process
	 * @param period - time between sensor readings. This will be used to estimate when the 
	 *	next reading should arrive, so out of order arrivals are
	 *	possible. Set to 0 if not a periodic stream. When set to a
	 *	negative value, the calculation of the buffer is performed for
	 *	that period, however no lookahead is set.
	 * @param bufferSize - The size of the internal FIFO buffer. This should be at least 
	 *  	the amount of samples that can occur in a timeout period. If no
	 *  	value is provided, the bufferSize is calculated from the period
	 *  	and timeout values provided, with an additional safety factor.
	 * @param priority - if streams have data with equal timestamps, the
	 *      one with the lower priority value will be pushed first.
	 *
	 * @result - stream index, which is used to identify the stream (e.g. for push).
	 */
	template <class T> int registerStream( boost::function<void (base::Time ts, T value)> callback, int bufferSize, base::Time period, int priority  = -1 ) 
	{
	    if( bufferSize < 1 )
	    {
		if( period == base::Time() )
		{
		    throw std::runtime_error("No buffer size provided for stream with unknown period.");
		}
		else if( period < base::Time() )
		{
		    // for a negative period, just calculate the buffer size, but don't set any lookahead.
		    bufferSize = buffer_size_factor * ceil( timeout.toSeconds() / -period.toSeconds() );
		    period = base::Time();
		}
		else
		{
		    bufferSize = buffer_size_factor * ceil( timeout.toSeconds() / period.toSeconds() );
		}
	    }

	    streams.push_back( new Stream<T>(callback, bufferSize, period, priority) );
	    return streams.size() - 1;
	}

	/** Push new data into the stream
	 * @param ts - the timestamp of the data item
	 * @param data - the data added to the stream
	 */
	template <class T> void push( int idx, base::Time ts, const T& data )
	{
	    if( idx < 0 )
		throw std::runtime_error("invalid stream index.");

	    // if not initialized, set current and latest time to the first
	    // ts that comes in
	    if( !initialized )
	    {
		current_ts = ts;
		latest_ts = ts;
		initialized = true;
	    }

	    // if ts is older than last ts that went out, its not added
	    // to the queue
	    if( ts < current_ts )
		return;

	    if( ts > latest_ts )
		latest_ts = ts;


	    Stream<T>* stream = dynamic_cast<Stream<T>*>(streams[idx]);
	    assert( stream );

	    stream->push( ts, data );
	}

	/** This will go through the available streams and look for the
	 * oldest available data. The data can be either existing are predicted
	 * through the period. 
	 * 
	 * There are three different cases that can happen:
	 *  - The data is already available. In this case that data is forwarded
	 *    to the callback.
	 *  - The data is not yet available, and the time difference between oldest
	 *    data and newest data is below the timeout threshold. In this case
	 *    no data is called.
	 *  - The data is not yet available, and the timeout is reached. In this
	 *    case, the oldest data (which is obviously non-available) is ignored,
	 *    and only newer data is considered.
	 *
	 *  @result - true if a callback was called and more data might be available 
	 */
	bool step()
	{
	    if( streams.empty() )
		return false;

	    // copy streams vector and sort it by next ts
	    stream_vector items = streams;
	    std::sort( items.begin(), items.end(), &compareStreams );

	    for(stream_vector::iterator it=items.begin();it != items.end();it++)
	    {
		if( (*it)->hasData() ) 
		{
		    // if stream has current data, pop that data
		    current_ts = (*it)->pop();
		    return true;
		}
		else if( latest_ts - current_ts < timeout )
		{
		    // if there is no data, but the expected data has
		    // not run out yet, wait for it.
		    return false;
		}
	    }
	    return false;
	}

	/** latency is the time difference between the latest data item that
	 * has come in, and the latest data item that went out
	 */
	base::Time getLatency() const { return latest_ts - current_ts; };

	/** return the time of the last data item that went out
	 */
	base::Time getCurrentTime() const { return current_ts; };

	/** return the time of the last data item that came in
	 */
	base::Time getLatestTime() const { return latest_ts; }

	/** return the buffer status as a std::pair. first element in pair is
	 * the current buffer fill and the second element is the buffer size 
	 */
	std::pair<size_t, size_t> getBufferStatus(int idx) const
	{
	    return streams[idx]->getBufferStatus();
	}

	friend std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner::StreamBase &base);
	friend std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner &re);
    };

    std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner &re)
    {
	using ::operator <<;
	stream << "current time: " << re.getCurrentTime() << " latest time:" << re.getLatestTime() << " latency: " << re.getLatency() << std::endl;
	for(size_t i=0;i<re.streams.size();i++)
	{
	    stream << i << ":" << *re.streams[i] << std::endl;
	}

	return stream;
    }

    std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner::StreamBase &base)
    {
	using ::operator <<;
	const std::pair<size_t, size_t> &status( base.getBufferStatus() );
	stream << status.first << "\t" << status.second << "\t" << base.nextTimeStamp();
	return stream;
    }
}

#endif
