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
        /** Total count of lost samples
         */
        int lost_samples_total;
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
        /** Losses that have been announced using updateLoss, but have yet to be
         * seen in the time stream
         */
        int expected_losses;
        /** The number of losses announced with updateLoss that got rejected by
         * the estimator
         */
        int rejected_expected_losses;

        TimestampEstimatorStatus()
            : lost_samples(0) {}
    };

    std::ostream& operator << (std::ostream& stream, TimestampEstimatorStatus const& status);
}

#endif

