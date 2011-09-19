#include "StreamAlignerStatus.hpp"

std::ostream &operator<<(std::ostream &os, const aggregator::StreamAlignerStatus &status)
{
    using ::operator <<;
    os
	<< "time agg: \t"
	<< "current time: \t" 
	<< " latest time: \t" 
	<< " dropped late samples: \t" << status.samples_dropped_late_arriving 
	<< " latency: \t" 
	<< std::endl
	<<  status.time
	<< "\t" << status.current_time 
	<< "\t" << status.latest_time 
	<< "\t" << status.samples_dropped_late_arriving 
	<< "\t" << status.latest_time - status.current_time 
	<< std::endl;
	
    if( status.streams.empty() )
    	return os; 
    
    os << "idx\tname\t\tbsize\tbfill\treceived\tprocessed\tdr_bfull\tdr_late\tbackward time" << std::endl;

    int cnt = 0;
    for(std::vector<aggregator::StreamStatus>::const_iterator it = status.streams.begin(); it != status.streams.end(); it++)
    {
	if(it->active)
	    os << cnt << "\t" << counters(os, *it); 
	cnt++;
    }
    
    os << "idx\tname\t\tlatest sample\tearliers data\tlatest data\tlatency" << std::endl;
    
    for(std::vector<aggregator::StreamStatus>::const_iterator it = status.streams.begin(); it != status.streams.end(); it++)
    {
	if(it->active)
	    os << cnt << "\t" << timers(os, *it, status.current_time); 
	cnt++;
    }
    return os;
}
std::ostream& counters(std::ostream& os, const aggregator::StreamStatus& status)
{
    using ::operator <<;
    os 	<< status.name << "\t\t"    
	<< status.buffer_size << "\t"
	<< status.buffer_fill << "\t"
	<< status.samples_received << "\t"
	<< status.samples_processed << "\t"
	<< status.samples_dropped_buffer_full << "\t"
	<< status.samples_dropped_late_arriving << "\t"
	<< status.samples_backward_in_time << "\t"
	<< std::endl;
    return os;
}

std::ostream& timers(std::ostream& os, const aggregator::StreamStatus& status, base::Time current_time)
{
    using ::operator <<;
    os 	
	<< status.latest_sample_time << "\t"
	<< status.earliest_data_time << " \t "
	<< status.latest_data_time << " \t " 
	<< status.latest_sample_time - current_time 
	<< std::endl;
    return os;
}


