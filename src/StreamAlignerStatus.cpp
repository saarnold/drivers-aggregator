#include "StreamAlignerStatus.hpp"

std::ostream &operator<<(std::ostream &os, const aggregator::StreamAlignerStatus &status)
{
    using ::operator <<;
    os
	<< "current time: " << status.current_time 
	<< " latest time: " << status.latest_time 
	<< " dropped late samples: " << status.late_arriving_samples_dropped 
	<< " latency: " << status.latest_time - status.current_time 
	<< std::endl;

    if( !status.streams.empty() )
    {
	os << "idx\tbsize\tbfill\tdrop_bfull\tdrop_late\tsample time\tstream time" << std::endl;
    }

    int cnt = 0;
    for(std::vector<aggregator::StreamStatus>::const_iterator it = status.streams.begin(); it != status.streams.end(); it++)
    {
	if(it->active)
	    os << cnt << "\t" << *it; 
	cnt++;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const aggregator::StreamStatus& status)
{
    using ::operator <<;
    os 	<< status.buffer_size << "\t"
	<< status.buffer_fill << "\t"
	<< status.samples_dropped_buffer_full << "\t"
	<< status.samples_dropped_late_arriving << "\t"
	<< status.latest_sample_time << "\t"
	<< status.latest_stream_time << std::endl;
    return os;
}
