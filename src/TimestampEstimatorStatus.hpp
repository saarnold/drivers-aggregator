#ifndef AGGREGATOR_TIMESTAMP_ESTIMATOR_STATUS_HPP
#define AGGREGATOR_TIMESTAMP_ESTIMATOR_STATUS_HPP

#include <base/time.h>

namespace aggregator {
    /** Structure used to report the internal state of a TimestampEstimator
     * instance
     */
    struct TimestampEstimatorStatus
    {
        /** When this data structure has been generated */
        base::Time stamp;
        /** Estimated period */
        base::Time period;
        /** Estimated latency (always null if no hardware timestamps are
         * provided)
         */
        base::Time latency;
        /** The maximum jitter value received so far
         */
        base::Time max_jitter;
        /** Count of lost samples currently stored in the estimator
         */
        int lost_samples;
        /** Count of samples currently stored in the estimator
         */
        int window_size;
        /** Maximum window capacity
         */
        int window_capacity;
        /** Time at which the base time got reset last
         */
        base::Time base_time;
        /** Offset at the last reset of base_time
         */
        base::Time base_time_reset_offset;

        TimestampEstimatorStatus()
            : lost_samples(0) {}
    };

    std::ostream& operator << (std::ostream& stream, TimestampEstimatorStatus const& status);
}

#endif

