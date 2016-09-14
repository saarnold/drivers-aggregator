#ifndef __AGGREGATOR_HPP__
#define __AGGREGATOR_HPP__

#include <base/Time.hpp>
#include <cmath>
#include <base-logging/Logging.hpp>
#include <vector>
#include <base/CircularBuffer.hpp>
#include <algorithm>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <stdexcept> 
#include <iostream>
#include <aggregator/StreamAlignerStatus.hpp>

namespace aggregator {

    class StreamAligner
    {
	class StreamBase
	{
	    friend class StreamAligner;
	    public:
		StreamBase() : active( true ) {}
		virtual ~StreamBase() {}
		virtual base::Time pop() = 0;
		virtual bool hasData() const = 0;
		virtual int getPriority() const = 0;
		virtual base::Time latestTimeStamp() const = 0;
		virtual base::Time latestDataTime() const = 0;
		virtual base::Time earliestDataTime() const = 0;
		virtual const StreamStatus &getBufferStatus() const = 0;
		virtual void copyState( const StreamBase& other ) = 0;
		virtual void clear() = 0;

		bool isActive() const { return active; }
		void setActive( bool active ) { this->active = active; }
		
		friend std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner::StreamBase &base);
		
	    protected:
		mutable StreamStatus status;
		/** marks a stream as active or inactive. All streams are active by default. */
		bool active;
	};

        public:
	template <class T> class Stream : public StreamBase
	{
	public:
	    typedef boost::function<void (const base::Time &ts, const T &value)> callback_t;

	protected:
	    typedef std::pair<base::Time,T> item;
	    boost::circular_buffer<item> buffer;
	    size_t bufferSize;
	    callback_t callback;
	    base::Time period; 
	    base::Time lastTime;
	    int priority;

	public:
	    Stream( callback_t callback, size_t bufferSize, base::Time period, int priority, const std::string &name )
		: bufferSize( bufferSize ), callback(callback), period(period), lastTime(base::Time::fromSeconds(0)), priority(priority)
            {
                status.name = name;
		status.priority = priority;

                if (bufferSize > 0)
                    buffer.set_capacity( bufferSize );
                else
                {
                    // initial size, will be reallocated at runtime
                    buffer.set_capacity( 20 );
                }
                status.buffer_size = buffer.capacity();
            }

	    virtual ~Stream() {};

	    bool getNextSample(item &sample) const
	    {
		if(buffer.empty())
		    return false;
		
		sample = buffer.front();
		return true;
	    }

	    virtual int getPriority() const
	    {
		return priority;
	    }

	    virtual const StreamStatus &getBufferStatus() const
	    {
		status.buffer_fill = buffer.size();
		status.latest_data_time = latestDataTime();
 		status.earliest_data_time = earliestDataTime();
		status.active = isActive();
		return status;
	    }

	    virtual void copyState( const StreamBase& other )
	    {
		const Stream<T> &stream(dynamic_cast<const Stream<T>& >(other));
		
		lastTime = stream.lastTime;
		buffer = stream.buffer;
		bufferSize = stream.bufferSize;
		status = stream.status; 
	    }

	    void push(const base::Time &ts, const T &data ) 
	    { 
		if(ts < lastTime)
		{
		    status.samples_backward_in_time++;
		    return;
		}
		
		lastTime = ts;

		if (buffer.full())
                {
		    if (bufferSize > 0)
		    {
		        // if the buffer is full, just use the behaviour of the circular
		        // buffer: discard old data.
		        status.samples_dropped_buffer_full++;
		    }
		    else
		    {
		        buffer.set_capacity(buffer.capacity() * 2);
			status.buffer_size = buffer.capacity();
		    }
		}
                buffer.push_back( std::make_pair(ts, data) ); 
	    }

	    /** take the last item of the stream queue and 
	     * call the callback 
	     */
	    base::Time pop() 
	    { 
		if( hasData() )
		{
		    status.samples_processed++;
		    base::Time ts = buffer.front().first;
		    if(callback)
			callback( ts, buffer.front().second );
		    buffer.pop_front();
		    return ts;
		}

		throw std::runtime_error("pop() called on stream with no data.");
	    }

	    bool hasData() const
	    { return !buffer.empty(); }

	    base::Time latestTimeStamp() const
	    {
		if( hasData() )
		    return buffer.front().first;
		else 
		    return lastTime + period;
	    }
	    
	    virtual base::Time latestDataTime() const
	    {
		return lastTime;
	    }
	    
	    virtual base::Time earliestDataTime() const
	    {
		if( hasData() )
		    return buffer.front().first;
		return base::Time();
	    }
	    
	    virtual void clear()
	    {	
		lastTime = base::Time();
		buffer.clear();
		
		status.latest_sample_time = base::Time();
		status.latest_data_time = base::Time();
		status.samples_dropped_buffer_full = 0;
		status.samples_dropped_late_arriving = 0;
		status.buffer_fill = 0;
		status.active = true;
	    };
	};

	static bool compareStreams( const StreamBase* b1, const StreamBase* b2 )
	{
	    if(!b1)
		return false;
	    
	    if(!b2)
		return true;
	    
	    const base::Time &ts1( b1->latestTimeStamp() );
	    const base::Time &ts2( b2->latestTimeStamp() );

	    if(ts1 == ts2)
	    {
		if(b1->hasData() && !b2->hasData())
		    return true;
		
		if(!b1->hasData() && b2->hasData())
		    return false;
		
		return b1->getPriority() < b2->getPriority();
	    }
	    
	    return ts1 < ts2;
	}

	typedef std::vector<StreamBase*> stream_vector;
	stream_vector streams;
	base::Time timeout;

	/** time of the last sample that came in */
	base::Time latest_ts;

	/** time of the last sample that went out */
	base::Time current_ts;

	double buffer_size_factor;

	/** temporary object that gets returned by getStatus, 
	 * in order to avoid dynamic allocation on each call
	 */  
	mutable StreamAlignerStatus status;

    public:
	explicit StreamAligner(base::Time timeout = base::Time::fromSeconds(1))
	    : timeout(timeout), buffer_size_factor(2.0) {}

	virtual ~StreamAligner()
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
	    latest_ts = other.latest_ts;
	    current_ts = other.current_ts;

	    assert( streams.size() == other.streams.size() );
	    for(size_t i=0;i<streams.size();i++)
	    {
		bool weGotStream = streams[i];
		bool otherGotStream = other.streams[i];
		if(weGotStream != otherGotStream)
		    throw std::runtime_error("Stream setup of second stream aligner differs");
		    
		if(streams[i])
		{
		    streams[i]->copyState( *other.streams[i] );
		}
	    }
	}

	/** Set the time the Estimator will wait for an expected reading on any of the streams.
	 * This number effectively puts an upper limit to the lag that can be created due to 
	 * delay or missing values on the channels.
	 */
	void setTimeout(const base::Time &t )
	{
	    timeout = t;
	}

	/** 
	 * Will disable the stream with the given index.  
	 *
	 * All data left in the stream will still be played out, however the
	 * stream will be ignored for lookahead and timeout calculation. A
	 * stream, which is disabled can be enabled through the enableStream()
	 * call, or if new data in this stream arrives.
	 *
	 * The functionality is needed for cases where streams might be
	 * optional, so that the other streams won't be delayed up to the
	 * maximum time out value.
	 */
	void disableStream( int idx )
	{
	    if(!streams[idx])
		throw std::runtime_error("invalid stream index.");		

	    streams[idx]->setActive( false );
	}

	/** 
	 * Enables a stream which has been disabled previously. 
	 *
	 * All streams are enabled by default. Does not have any effect on
	 * streams which are already enabled.
	 */
	void enableStream( int idx )
	{
	    if(!streams[idx])
		throw std::runtime_error("invalid stream index.");		

	    streams[idx]->setActive( true );
	}

	/** 
	 * See if a stream is enabled or disabled.
	 *
	 * @return true if the stream is enabled (active)
	 */
	bool isStreamActive( int idx ) const 
	{
	    if(!streams[idx])
		throw std::runtime_error("invalid stream index.");		

	    return streams[idx]->isActive();
	}

	/**
	 * This function will remove the stream with the given index from the
	 * stream aligner.
	 * 
	 * @param idx index of the stream that should be unregistered 
	 */
	void unregisterStream(int idx)
	{
	    if(!streams[idx])
	    {
		throw std::runtime_error("invalid stream index.");		
	    }
	    
	    delete streams[idx];
	    
	    streams[idx] = 0;
	    
	    status.streams[idx].active = false;
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
	 * @param name - name of the stream. This is only for debug purposes
	 * 
	 * @result - stream index, which is used to identify the stream (e.g. for push).
	 */
	template <class T> int registerStream( typename Stream<T>::callback_t callback, int bufferSize, base::Time period, int priority  = -1, const std::string &name = std::string()) 
	{
	    if( bufferSize < 0 )
	    {
		if( period == base::Time() )
		{
		    throw std::runtime_error("No buffer size provided for stream with unknown period.");
		}
		else if( period < base::Time() )
		{
		    // for a negative period, just calculate the buffer size, but don't set any lookahead.
		    bufferSize = buffer_size_factor * std::ceil( timeout.toSeconds() / -period.toSeconds() );
		    period = base::Time();
		}
		else
		{
		    bufferSize = buffer_size_factor * std::ceil( timeout.toSeconds() / period.toSeconds() );
		}
	    }

	    if( bufferSize == 0 )
	    {
		LOG_DEBUG_S << "dynamically allocating stream aligner buffer for stream: " << name;
	    }

	    StreamBase *newStream = new Stream<T>(callback, bufferSize, period, priority, name);
	    
	    //check if there is a free slot from a previous deleted stream
	    for(size_t i = 0; i < streams.size(); i++)
	    {
		if(!streams[i])
		{
		    streams[i] = newStream;
		    status.streams[i] = StreamStatus();
		    return i;
		}
	    }
		
	    streams.push_back( newStream );
	    status.streams.push_back(StreamStatus());
	    return streams.size() - 1;
	}
	
	/** @brief Push new data into the stream
	 *
	 * Note that if the stream was previously inactive, this call will make
	 * it active implicetely.
	 *
	 * @param ts - the timestamp of the data item
	 * @param data - the data added to the stream
	 */
	template <class T> void push( int idx, const base::Time &ts, const T& data )
	{
	    if( !streams.at(idx) )
		throw std::runtime_error("invalid stream index.");

	    Stream<T>* stream = dynamic_cast<Stream<T>*>(streams[idx]);
	    assert( stream );

	    stream->status.samples_received++;
	    stream->status.latest_sample_time = ts;

	    // mark stream as active, since it is receiving data items will
	    // have no effect on an already active stream, but enables
	    // streams which have been marked passive before.
	    stream->setActive( true );

	    //any sample, that is older than the last replayed sample
	    //will never be played back and gets dropped by default
	    if(ts < current_ts) 
	    {
		status.samples_dropped_late_arriving++;
		stream->status.samples_dropped_late_arriving++;
		return;
	    }

	    if( ts > latest_ts )
		latest_ts = ts;
	    
	    stream->push( ts, data );
	}

	template <class T> bool getNextSample( int idx, std::pair<base::Time,T> &sample) const
	{
	    if( !streams.at(idx) )
		throw std::runtime_error("invalid stream index.");

	    Stream<T>* stream = dynamic_cast<Stream<T>*>(streams[idx]);
	    assert( stream );

	    return stream->getNextSample(sample);
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
		//first stream is unregistered no data there
		if(!*it)
		    return false;
		
		if( (*it)->hasData() ) 
		{
		    // if stream has current data, pop that data
		    current_ts = (*it)->pop();
		    return true;
		}
		else if( (*it)->isActive() )
		{

		    base::Time latestDataTime;
		    base::Time firstDataTime;
		    
		    //initalization case
		    if(current_ts == base::Time())
		    {
			//check if one stream timed out
			for(stream_vector::iterator it2=items.begin();it2 != items.end();it2++)
			{
			    
			    if(*it2 && (*it2)->hasData())
			    {
				if(latestDataTime < (*it2)->latestDataTime())
				    latestDataTime = (*it2)->latestDataTime();
				
				if(firstDataTime == base::Time() || firstDataTime > (*it2)->earliestDataTime())
				    firstDataTime = (*it2)->earliestDataTime();
			    }
			}			
		    } else {
			latestDataTime = latest_ts;
			firstDataTime = current_ts;
		    }

		    if(latestDataTime - firstDataTime < timeout)
		    {
			// if there is no data, but the expected data has
			// not run out yet, wait for it.
			return false;
		    }
		}
	    }
	    return false;
	}

	/**
	 * clears all samples in all streams, resets the statistics
	 * and resets the playback times  but leaves the stream
	 * setup intact.
	 */
	void clear()
	{
	    for(size_t i = 0; i < streams.size(); i++)
	    {
		if(streams[i])
		{
		    streams[i]->clear();
		}
	    }
	    
	    latest_ts = base::Time();
	    current_ts = base::Time();
	    
	    status.current_time = base::Time();
	    status.latest_time = base::Time();
	    status.samples_dropped_late_arriving = 0;
	}

	/** Get the time the Estimator will wait for an expected reading on any of the streams.
	 * This number effectively puts an upper limit to the lag that can be created due to 
	 * delay or missing values on the channels.
	 */
	base::Time getTimeOut() const { return timeout; };
	
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

	/** return the number of streams 
	*/
	int getStreamSize() const { return streams.size();  } 

	/** return the buffer status as a std::pair. first element in pair is
	 * the current buffer fill and the second element is the buffer size 
	 */
	const StreamStatus &getBufferStatus(int idx) const
	{
	    if( !streams.at(idx) )
		throw std::runtime_error("invalid stream index.");
	    
	    return streams[idx]->getBufferStatus();
	}

	/** @return the current status of the StreamAligner
	 * this is mainly used for debug purposes
	 */
	const StreamAlignerStatus& getStatus() const 
	{
	    status.time = base::Time::now();
	    status.current_time = getCurrentTime();
	    status.latest_time = getLatestTime();

	    for(size_t i=0;i<streams.size();i++)
	    {
		if(streams[i])
		    status.streams[i] = streams[i]->getBufferStatus();
	    }

	    return status;
	}

	friend std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner::StreamBase &base);
	friend std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner &re);
    };

    inline std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner &re)
    {
	using ::operator <<;
	stream << "current time: " << re.getCurrentTime() << " latest time:" << re.getLatestTime() << " latency: " << re.getLatency() << std::endl;
	for(size_t i=0;i<re.streams.size();i++)
	{
	    stream << i << ":" << *re.streams[i] << std::endl;
	}

	return stream;
    }

    inline std::ostream &operator<<(std::ostream &stream, const aggregator::StreamAligner::StreamBase &base)
    {
	using ::operator <<;
	stream << base.getBufferStatus();
	return stream;
    }
}

#endif
