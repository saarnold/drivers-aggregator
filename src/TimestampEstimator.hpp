#ifndef AGGREGATOR_TIMESTAMP_ESTIMATOR_HPP
#define AGGREGATOR_TIMESTAMP_ESTIMATOR_HPP

#include <base/time.h>
#include <list>
#include <vector>

namespace aggregator
{
    /** The timestamp estimator takes a stream of samples and determines a best
     * guess for each of the samples timestamp.
     *
     * It assumes that most samples will be received at the right period. It
     * will not work if the reception period is completely random.
     */
    class TimestampEstimator
    {
        /** The requested estimation window */
        double m_window;

        /** The currently stored timestamps */
        std::list<double> m_samples;

        /** The last estimated timestamp */
        double m_last;

        /** if m_lost is greater than m_lost_threshold, we consider that we lost
         * some samples
         */
        int m_lost_threshold;

        /** The number of successive calls to update() where we could have lost
         * a sample */
        std::vector<int> m_lost;

        /** The total estimated count of lost samples so far */
        int m_lost_total;

        double getPeriodInternal() const;

        double m_min_offset;
        double m_min_offset_reset;

    public:
        /** Creates a timestamp estimator
         *
         * @arg window the size of the window that should be used to the
         *        estimation. It should be an order of magnitude smaller
         *        than the period drift in the estimated stream
         *
         * @arg lost_threshold if that many calls to update() are out of bounds
         *        (i.e. the distance between the two timestamps are greater than
         *        the period), then we consider that we lost samples and update
         *        the timestamp accordingly. Set to 0 if you are sure that the
         *        acquisition latency is lower than the device period.
         */
        TimestampEstimator(base::Time window, int lost_threshold = 2);

        /** Updates the estimate and return the actual timestamp for +ts+ */
        base::Time update(base::Time ts);

        /** The currently estimated period
         */
        base::Time getPeriod() const;

        /** The total estimated count of lost samples so far */
        int getLostSampleCount() const;
    };
}

#endif

