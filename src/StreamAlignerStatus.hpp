#ifndef __AGGREGATOR__STREAMALIGNERSTATUS_HPP__
#define __AGGREGATOR__STREAMALIGNERSTATUS_HPP__

#include <base/Time.hpp>
#include <vector>

namespace aggregator 
{
    /** Debugging structure used to report about the status of a single stream in a stream aligner
     */
    struct StreamStatus
    {
	/** The actual size of the buffer */
	size_t buffer_size;
	/** How many samples are currently waiting inside the stream buffer */
	size_t buffer_fill;
	/** The total number of samples ever received for that stream
	 * 
	 * The following relationship should hold:
	 *   
	 *   samples_received == samples_processed +
	 * 	samples_dropped_buffer_full +
	 * 	samples_dropped_late_arriving
	 */
	size_t samples_received;
	/** The total count of samples ever processed by the callbacks of this stream
	 * 
	 * The total number of samples ever received is
	 *   
	 *   samples_processed + samples_dropped_buffer_full + samples_dropped_late_arriving
	 */
	size_t samples_processed;
	/** Count of samples dropped because the buffer was full
	 * 
	 * Should be zero on streams that have dynamically resized buffers
	 */
	size_t samples_dropped_buffer_full;
	/** Count of samples dropped because their timestamp was earlier than
	 * the stream aligner current time
	 */
	size_t samples_dropped_late_arriving;
	/** Count of samples dropped because their timestamp was not properly ordered
	 * 
	 * I.e. samples for which the timestamp was later than the previous
	 * sample received for that stream
	 */
	size_t samples_backward_in_time;
	/** Time of the newest sample currently stored in the stream buffer.
	 * Null time if the stream is empty
	 */
	base::Time latest_data_time;
	/** Time of the oldest sample currently stored in the stream buffer.
	 * Null time if the stream is empty
	 */
	base::Time earliest_data_time;	
	/** Time of the last sample received for this stream, regardless of
	 * whether it has been dropped or pushed to the stream
	 */
	base::Time latest_sample_time;
	/** True if the stream is being used by the stream aligner */
	bool active;
	/** The stream name. In the case of the oroGen plugin, this is set to
	 * the port name
	 */
	std::string name;
	/** The priority at which this stream is processed. When samples of the
	 * same timestamp are available on two different streams, the stream
	 * with the lower priority value is processed first.
	 */
	int64_t priority;
	
	StreamStatus() : buffer_size(0), buffer_fill(0), samples_received(0), 
			samples_processed(0), samples_dropped_buffer_full(0), 
			samples_dropped_late_arriving(0), 
			samples_backward_in_time(0), active(true), priority(0)
	{
	}
    };

    /** Structure used to report the complete state of a stream aligner
     * 
     * The stream aligner latency is time - current_time
     */
    struct StreamAlignerStatus
    {
	/** Time at which this data structure got generated
	 */
	base::Time time;
	/** The name of the stream aligner. In the case of the oroGen plugin,
	 * this is the name of the underlying oroGen task
	 */
	std::string name;
	/** The stream aligner's time
	 * 
	 * This is the time of the last sample that has been given to a stream
	 * aligner callback
	 */
	base::Time current_time;
	/** Time of the last sample that got in the stream aligner
	 */
	base::Time latest_time;
	/** Count of samples that got dropped because, at the time they arrived,
	 * they were older than the stream aligner's current time.
	 * 
	 * This happens if: the stream aligner timed out or if a sample arrived
	 * earlier than the stream's declared period (i.e. the period is too big).
	 */
	size_t samples_dropped_late_arriving;
	/** Status of each individual streams
	 */
	std::vector<StreamStatus> streams;
	
	StreamAlignerStatus() : samples_dropped_late_arriving(0)
	{
	}	
    };

}

std::ostream &operator<<(std::ostream &os, const aggregator::StreamAlignerStatus &status);
std::ostream &operator<<(std::ostream &os, const aggregator::StreamStatus &status);
std::ostream& counters(std::ostream& os, const aggregator::StreamStatus& status);
std::ostream& timers(std::ostream& os, const aggregator::StreamStatus& status, base::Time current_time);

#endif
