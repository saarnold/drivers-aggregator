#include "StreamAlignerStatus.hpp"

std::ostream &operator<<(std::ostream &os, const aggregator::StreamAlignerStatus &status)
{
    using ::operator <<;
    os
	<< "current time: " << status.current_time 
	<< " latest time: " << status.latest_time 
	<< " latency: " << status.latest_time - status.current_time 
	<< std::endl;

    if( !status.streams.empty() )
    {
	os << "idx\tbsize\tbfill\tlatest time" << std::endl;
    }

    for( size_t i=0; i<status.streams.size(); i++ )
    {
	const aggregator::StreamStatus &stream(status.streams[i]);
	os << i << "\t" 
	    << stream.buffer_size << "\t"
	    << stream.buffer_fill << "\t"
	    << stream.latest_time << std::endl;
    }
    return os;
}
