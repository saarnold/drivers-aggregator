#ifndef __AGGREGATOR__STREAMALIGNERSTATUS_HPP__
#define __AGGREGATOR__STREAMALIGNERSTATUS_HPP__

#include <base/time.h>
#include <vector>

namespace aggregator 
{
    struct StreamStatus
    {
	size_t buffer_size;
	size_t buffer_fill;
	size_t samples_dropped_buffer_full;
	size_t samples_dropped_late_arriving;
	base::Time latest_stream_time;	
	base::Time latest_sample_time;
	bool active;
	
	StreamStatus(): buffer_size(0), buffer_fill(0), samples_dropped_buffer_full(0), samples_dropped_late_arriving(0), active(true)
	{
	}
    };

    struct StreamAlignerStatus
    {
	base::Time current_time;
	base::Time latest_time;
	size_t late_arriving_samples_dropped;
	std::vector<StreamStatus> streams;
	
	StreamAlignerStatus() : late_arriving_samples_dropped(0)
	{
	}	
    };

}

std::ostream &operator<<(std::ostream &os, const aggregator::StreamAlignerStatus &status);
std::ostream &operator<<(std::ostream &os, const aggregator::StreamStatus &status);
#endif
