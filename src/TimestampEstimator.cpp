#include "TimestampEstimator.hpp"
#include <limits.h> //for INT_MAX
#include <iosfwd>
#include <stdexcept>
#include <iostream>
#include <base/float.h>

using namespace aggregator;

TimestampEstimator::TimestampEstimator(base::Time window,
				       base::Time initial_period,
				       base::Time initial_latency,
				       int lost_threshold)
{
    reset(window, initial_period, initial_latency, lost_threshold);
}

TimestampEstimator::TimestampEstimator(base::Time window,
				       base::Time initial_period,
				       int lost_threshold)
{
    reset(window, initial_period, base::Time(), lost_threshold);
}

TimestampEstimator::TimestampEstimator(base::Time window,
				       int lost_threshold)
{
    reset(window, base::Time(), base::Time(), lost_threshold);
}

void TimestampEstimator::reset()
{
    internalReset(m_window,
            m_initial_period,
            m_initial_latency,
            m_lost_threshold);
}

void TimestampEstimator::reset(base::Time window,
				       int lost_threshold)
{
    internalReset(window.toSeconds(),
            m_initial_period,
            m_initial_latency,
            lost_threshold);
}

void TimestampEstimator::reset(base::Time window,
				       base::Time initial_period,
				       int lost_threshold)
{
    internalReset(window.toSeconds(),
            initial_period.toSeconds(),
            m_initial_latency,
            lost_threshold);
}

void TimestampEstimator::reset(base::Time window,
				       base::Time initial_period,
				       base::Time initial_latency,
				       int lost_threshold)
{
    internalReset(window.toSeconds(),
            initial_period.toSeconds(),
            initial_latency.toSeconds(),
            lost_threshold);
}

void TimestampEstimator::internalReset(double window,
				       double initial_period,
				       double initial_latency,
				       int lost_threshold)
{
    m_got_full_window = false;
    m_zero = base::Time();
    m_window = window;
    m_lost_threshold = lost_threshold;
    m_lost.clear();
    m_lost_total = 0;
    m_base_time_reset = 0;
    m_max_jitter = 0;
    m_latency = initial_latency;
    m_initial_latency = initial_latency;
    m_initial_period = initial_period;
    m_missing_samples = 0;
    m_last_index = 0;
    m_have_last_index = false;

    m_samples.clear();
    if (m_initial_period > 0)
        m_samples.set_capacity(10 + (m_window + m_initial_period) / m_initial_period);
    else
        m_samples.set_capacity(20); // should be enough to get us a first period estimate
}

base::Time TimestampEstimator::getPeriod() const
{ return base::Time::fromSeconds(getPeriodInternal()); }
double TimestampEstimator::getPeriodInternal() const
{
    if (!m_got_full_window && m_initial_period)
    {
        // The main problem with using an initial period is that the estimator
        // gets lost if the period is under-estimated.
        //
        // Other ways to handle the initial period:
        //  * fill the sample set using the first sample and the period. In the
        //    end, it will be equal to this solution plus
        //    ((jitter_on_last_sample - jitter_on_very_first_sample) / window_size_in_periods)
        //    Since the first sample, in this case, does not change, it skews
        //    the period estimate for a complete first window
        //  * compute the period using latest - (m_last - m_initial_period *
        //    window_size_in_period). It is identical to this solution plus
        //    jitter_on_current_sample / window_size_in_periods. This has a
        //    randomness, but does not fix the behaviour if the initial period
        //    is under-estimated.
        //
        // So, go for the simple solution and document for the user that the
        // initial period should be very slightly over-estimated (if possible).
        return m_initial_period;
    }
    else
    {
        int count = m_samples.size();
        boost::circular_buffer<double>::const_reverse_iterator latest_it;
        //ignore lost samples(unset value) at the end of m_samples
        for(latest_it = m_samples.rbegin();
                base::isUnset(*latest_it) && latest_it != m_samples.rend();
                latest_it++, count--)
        {}
        double latest = *latest_it;

        if (count <= 1)
            throw std::logic_error("getPeriodInternal() called with no initial period and less than 2 valid samples");

        double earliest = m_samples.front();
        return (latest - earliest) / (count - 1);
    }
}

int TimestampEstimator::getLostSampleCount() const
{ return m_lost_total; }

void TimestampEstimator::shortenSampleList(double current)
{
    if (haveEstimate())
    {
	// Compute the period up to now for later reuse
	double period = getPeriodInternal();

	//scan forward until we hit the window size
        boost::circular_buffer<double>::iterator end = m_samples.begin();
	double min_time = current - m_window;
	while(end != m_samples.end() && (base::isUnset(*end) || *end < min_time))
        {
            if (!base::isUnset(*end))
                m_got_full_window = true;
	    end++;
        }

        if (end == m_samples.end())
        {
            m_samples.clear();
            m_missing_samples = 0;
            return;
        }

        boost::circular_buffer<double>::iterator window_begin = end;

	//scan backward until we find a gap that is at least period sized.
	//that should be the last sample from a burst, giving better
	//period estimation
        //
        //The 0.9 factor on the period is here to allow a bit of jitter.
        //Otherwise, we might end up keeping too much data for too long
        boost::circular_buffer<double>::iterator last_good = end;
	int smp_count = 0;
	while(end != m_samples.begin())
	{
	    if (*end > 0)
	    {
		if (smp_count > 0 && (*last_good-*end) / smp_count >= 0.9 * period)
		    break;

		last_good = end;
		smp_count = 0;
	    }
	    end--;
	    smp_count++;
	}

	//if we didn't find anything and the buffer is too large already,
	//fall back to real window begin
	if (end == m_samples.begin() && *end < min_time - m_window)
	    end = window_begin;

	//scan forward again as long as we find lost samples
	for(;end != m_samples.end() && base::isUnset(*end); end++) {}

        boost::circular_buffer<double>::iterator it;
	for(it = m_samples.begin(); it != end; it++) {
	    if (base::isUnset(*it))
		m_missing_samples--;
	}

	m_samples.erase(m_samples.begin(), end);
    }

    if (m_samples.size() == m_missing_samples)
    {
	m_samples.clear();
	m_missing_samples = 0;
    }
}

base::Time TimestampEstimator::update(base::Time time)
{
    if (m_zero.isNull())
        m_zero = time;

    // We use doubles internally. Convert to it.
    double current = (time - m_zero).toSeconds();

    // Remove values from m_samples that are outside the required window
    shortenSampleList(current);

    // If we have an initial period, fill m_samples using it
    if (m_samples.empty())
    {
        m_last = current;
        m_base_time_reset = m_last;
        m_samples.push_back(current);
        return base::Time::fromSeconds(m_last - m_latency) + m_zero;
    }

    // If we have an initial period, m_samples has been sized already. Since
    // push_back will override the beginning of the circular buffer, there is
    // nothing to do if the buffer is full
    //
    // If we don't have an initial period, however, we have to dynamically
    // update its capacity using the current period estimate.
    if (m_samples.full() && !m_initial_period)
    {
        if (haveEstimate())
        {
            double period = getPeriodInternal();
            size_t new_capacity = 10 + (m_window + period) / period;
            if (m_samples.capacity() < new_capacity)
                m_samples.set_capacity(new_capacity);
        }
        else
        {
            m_samples.set_capacity(20 + m_samples.capacity());
        }
    }

    // Add the new input to the sample set
    m_samples.push_back(current);

    // Not enough samples, just return the input value.
    if (!haveEstimate())
        return base::Time::fromSeconds(m_last - m_latency) + m_zero;

    // Recompute the period
    double period = getPeriodInternal();

    // To avoid long-term effects of estimation errors, the base time must be
    // updated at least once in a time window.
    //
    // In principle, it should not happen
    if (current - m_base_time_reset > m_window)
    {
        m_last = current;
        m_base_time_reset = current;
    }

    // Check for lost samples
    //
    // This is the distance, in periods, between the last received timestamp and
    // the one we just received
    //
    // What m_lost_threshold represents is the number of samples that seem to be
    // lost that are required before we decide that we indeed did lose a sample.
    // This is made so that we are robust to losing a single sample even if
    // m_lost_threshold > 10 (for instance)
    //
    // The issue here is that, if e.g. 10 samples are lost at once, then the
    // estimator will take 10 * m_lost_threshold samples to recover
    if (m_lost_threshold != INT_MAX)
    {
        int sample_distance = (current - m_last) / period;
        if (sample_distance > 1)
            m_lost.push_back(sample_distance - 1);
        else
            m_lost.clear();

        if (static_cast<int>(m_lost.size()) > m_lost_threshold)
        {
            int lost_count = *std::min_element(m_lost.begin(), m_lost.end());
            for (int i = 0; i < lost_count; ++i)
                updateLoss();
        }
    }

    // m_last is tracking the current base time, i.e. the best estimate for the
    // next sample is always m_last + period
    //
    // If this condition is true, it means that the current time stream is
    // actually too late (we are receiving a sample that is earlier than m_last
    // + period). We therefore need to update m_last to reflect that fact.
    //
    // To avoid resetting the base time unnecessarily, consider that we
    // "reset" it as soon as we are within 1e-4 of it.
    if (m_last + period > current - period * 1e-4)
    {
        m_last = current;
        m_base_time_reset = current;
    }
    else
        m_last = m_last + period;

    m_max_jitter = std::max(static_cast<double>(m_max_jitter), current - m_last);
    return base::Time::fromSeconds(m_last - m_latency) + m_zero;
}

base::Time TimestampEstimator::updateLoss()
{
    m_samples.push_back(base::unset<double>());
    m_missing_samples++;

    if (haveEstimate()) {
	double period = getPeriodInternal();
	m_last = m_last + period;
    }
    return base::Time::fromSeconds(m_last - m_latency) + m_zero;
}

void TimestampEstimator::updateReference(base::Time ts)
{
    if (!haveEstimate())
	return;

    double period = getPeriodInternal();
    double hw_time   = (ts - m_zero).toSeconds();
    double est_time = m_last - m_latency;
    int n = round((est_time - hw_time)/period);
    double diff = est_time - (hw_time + n * period);

    m_latency += diff;
    m_last    += diff;
}

bool TimestampEstimator::haveEstimate() const
{
    if (m_initial_period)
        return (m_samples.size() - m_missing_samples) >= 1;
    else
        return (m_samples.size() - m_missing_samples) >= 2;
}

base::Time TimestampEstimator::update(base::Time time, int64_t index)
{
    if (!m_have_last_index || index < m_last_index)
    {
	m_have_last_index = true;
        m_last_index = index;
        return update(time);
    }

    int64_t lost = index - m_last_index - 1;
    m_last_index = index;
    while(lost > 0)
    {
	lost--;
	updateLoss();
    }
    return update(time);
}

base::Time TimestampEstimator::getLatency() const
{
    return base::Time::fromSeconds(m_latency);
}

base::Time TimestampEstimator::getMaxJitter() const
{
    return base::Time::fromSeconds(m_max_jitter);
}

TimestampEstimatorStatus TimestampEstimator::getStatus() const
{
    TimestampEstimatorStatus status;
    status.stamp = base::Time::fromSeconds(m_last - m_latency) + m_zero;
    status.period = getPeriod();
    status.latency = getLatency();
    status.max_jitter = getMaxJitter();
    status.lost_samples = getLostSampleCount();
    status.window_size = m_samples.size();
    return status;
}

std::ostream& aggregator::operator << (std::ostream& stream, TimestampEstimatorStatus const& status)
{
    stream << "== Timestamp Estimator Status\n"
        << "stamp: " << status.stamp.toSeconds() << "\n"
        << "period: " << status.period.toSeconds() << "\n"
        << "latency: " << status.latency.toSeconds() << "\n"
        << "max_jitter: " << status.max_jitter.toSeconds() << "\n"
        << "lost_samples: " << status.lost_samples << "\n"
        << "window_size: " << status.window_size << std::endl;
    return stream;
}

