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
        /** Count of lost samples so far
         */
        int lost_samples;

        TimestampEstimatorStatus()
            : lost_samples(0) {}
    };
}

#endif

