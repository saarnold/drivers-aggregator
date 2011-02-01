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
	base::Time latest_time;
    };

    struct StreamAlignerStatus
    {
	base::Time current_time;
	base::Time latest_time;
	std::vector<StreamStatus> streams;
    };

}

std::ostream &operator<<(std::ostream &os, const aggregator::StreamAlignerStatus &status);
#endif
