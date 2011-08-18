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

        /** The currently stored timestamps. values < 0 are placeholders for
	 *  missing samples
	 */
        std::list<double> m_samples;

        /** The last estimated timestamp */
        long double m_last;

        /** if m_lost.size() is greater than m_lost_threshold, we consider
	 * that we lost some samples
         */
        int m_lost_threshold;

        /** The number of successive samples put into update() where we could
	 * have lost another sample */
	std::vector<int> m_lost;

        /** The total estimated count of lost samples so far */
        int m_lost_total;

        double getPeriodInternal() const;

        double m_min_offset;
        double m_min_offset_reset;
	double m_latency;
	double m_min_latency;

	/** Initial period used when m_samples is empty */
	double m_initial_period;

	/** number of missing samples recorded in m_samples */
	unsigned int m_missing_samples;

	/** the value of the last index given to us using update */
	unsigned int m_last_index;

	/** m_last_index is initialized */
	bool m_have_last_index;
    public:
        /** Creates a timestamp estimator
         *
         * @arg window the size of the window that should be used to the
         *        estimation. It should be an order of magnitude smaller
         *        than the period drift in the estimated stream
         *
	 * @arg initial_period initial estimate for the period, used to fill
	 *        the initial window.
	 *
	 * @arg min_latency the smallest amount of latency between the
	 *        reference timestamps and the data timestamps
	 *
         * @arg lost_threshold if that many calls to update() are out of bounds
         *        (i.e. the distance between the two timestamps are greater than
         *        the period), then we consider that we lost samples and update
         *        the timestamp accordingly. Set to 0 if you are sure that the
         *        acquisition latency is lower than the device period.
	 *        Set to INT_MAX if you are sure to either not lose any samples
	 *        or know about all lost samples and use updateLoss()/
	 *        update(base::Time,int)
         */
	TimestampEstimator(base::Time window,
			   base::Time initial_period,
			   base::Time min_latency,
			   int lost_threshold = 2);
	TimestampEstimator(base::Time window,
			   base::Time initial_period,
			   int lost_threshold = 2);
	TimestampEstimator(base::Time window,
			   int lost_threshold = 2);

        /** Resets this estimator to an initial state, reusing the same
         * parameters
         *
         * See the constructor documentation for parameter documentation
         */
	void reset();

        /** Changes the estimator parameters, and resets it to an initial state
         *
         * See the constructor documentation for parameter documentation
         */
	void reset(base::Time window,
			   base::Time initial_period,
			   base::Time min_latency,
			   int lost_threshold);

        /** Updates the estimate and return the actual timestamp for +ts+ */
        base::Time update(base::Time ts);

        /** Updates the estimate and return the actual timestamp for +ts+,
	 *  calculating lost samples from the index
	 */
	base::Time update(base::Time ts, int index);

        /** Updates the estimate for a known lost sample */
	base::Time updateLoss();

        /** Updates the estimate using a reference */
	void updateReference(base::Time ts);

        /** The currently estimated period
         */
        base::Time getPeriod() const;

	/** Shortens the sample list to account for the current timestamp
	 *  time. Calling this is strongly recommended if there is a chance
	 *  of only calling updateLoss for long stretches of time
	 */
        void shortenSampleList(base::Time time);

        /** The total estimated count of lost samples so far */
        int getLostSampleCount() const;

        /** Returns true if updateLoss and getPeriod can give valid estimates */
	bool haveEstimate() const;
    };
}

#endif

