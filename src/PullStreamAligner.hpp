#ifndef __AGGREGATORE_PULLSTREAMALIGNER__
#define __AGGREGATORE_PULLSTREAMALIGNER__

#include <aggregator/StreamAligner.hpp>

namespace aggregator
{
    class PullStreamAligner : public StreamAligner
    {
	class PullStreamBase 
	{
	public:
	    PullStreamBase() : has_data(false) {}
	    virtual ~PullStreamBase() {}
	    virtual void pull() = 0;
	    virtual void push() = 0; 
	    virtual void copyState( const PullStreamBase& other ) = 0; 

	    base::Time lastTime() const { return last_ts; }
	    bool hasData() const { return has_data; }

	protected:
	    base::Time last_ts;
	    bool has_data;
	};

	template <class T> class PullStream : public PullStreamBase 
	{
	public:
	    typedef boost::function<bool (base::Time&, T&)> pull_callback_t;

	    PullStream( pull_callback_t pull_callback, StreamAligner* sa, size_t stream_index )
		: stream_idx( stream_index ), sa( sa ), pull_callback( pull_callback ) {}

	    void pull()
	    {
		has_data = pull_callback( last_ts, last_data );
	    }

	    void push()
	    {
		if( has_data )
		    sa->push( stream_idx, last_ts, last_data );

		has_data = false;	
	    }

	    void copyState( const PullStreamBase& other )
	    {
		const PullStream<T> &pull_stream(static_cast<const PullStream<T>& >(other));
		operator=( pull_stream );
	    }

	protected:
	    int stream_idx;
	    StreamAligner *sa;

	    pull_callback_t pull_callback;
	    T last_data;
	};

	static bool comparePullStreams( const PullStreamBase* b1, const PullStreamBase* b2 )
	{
	    const base::Time &ts1( b1->lastTime() );
	    const base::Time &ts2( b2->lastTime() );
	    return ts1 < ts2 || !b2->hasData(); 
	}

    public:
	template <class T> 
	int registerStream( typename PullStream<T>::pull_callback_t pull_callback, 
		typename Stream<T>::callback_t callback, int bufferSize, base::Time period, int priority  = -1 ) 
	{
	    int idx = StreamAligner::registerStream<T>( callback, bufferSize, period, priority );
	    pull_streams.push_back( new PullStream<T>( pull_callback, this, idx ) );
	    return idx;
	}

	bool pull()
	{
	    for(pull_stream_vector::iterator it=pull_streams.begin();it != pull_streams.end();it++)
	    {
		if( !(*it)->hasData() )
		    (*it)->pull();
	    }
	    std::sort( pull_streams.begin(), pull_streams.end(), &comparePullStreams );

	    PullStreamBase *first = pull_streams.front();
	    if( first->hasData() )
	    {
		first->push();
		return true;
	    }
	    return false;
	}

	void copyState(const PullStreamAligner& other)
	{
	    StreamAligner::copyState( other );

	    assert( pull_streams.size() == other.pull_streams.size() );
	    for(size_t i=0;i<pull_streams.size();i++)
	    {
		pull_streams[i]->copyState( *other.pull_streams[i] );
	    }
	}

	~PullStreamAligner()
	{
	    for(pull_stream_vector::iterator it=pull_streams.begin();it != pull_streams.end();it++)
		delete *it;
	}

    protected:
	typedef std::vector<PullStreamBase*> pull_stream_vector;
	pull_stream_vector pull_streams;
    };
}

#endif
