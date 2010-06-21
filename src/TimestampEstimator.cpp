#include "TimestampEstimator.hpp"
#include <iostream>
#include <stdexcept>

using namespace aggregator;

TimestampEstimator::TimestampEstimator(base::Time window, int lost_threshold)
    : m_window(window.toSeconds()), m_lost_threshold(lost_threshold)
    , m_lost(0), m_lost_total(0), m_min_offset(0), m_min_offset_reset(0)
{
}

base::Time TimestampEstimator::getPeriod() const
{ return base::Time::fromSeconds(getPeriodInternal()); }
double TimestampEstimator::getPeriodInternal() const
{ return (m_samples.back() - m_samples.front()) / (m_samples.size() - 1); }
int TimestampEstimator::getLostSampleCount() const
{ return m_lost_total; }

base::Time TimestampEstimator::update(base::Time time)
{
    double current = time.toSeconds();

    // Remove the samples in m_samples that are outside our estimation window
    double min_time = current - m_window;
    while (!m_samples.empty() && m_samples.front() < min_time)
        m_samples.pop_front();

    // Add the new sample, and return if we don't have at least two samples
    m_samples.push_back(current);
    if (m_samples.size() < 2)
    {
        m_last = current;
        m_min_offset_reset = current;
        return time;
    }

    // Compute the period
    double period = getPeriodInternal();

    // Update the base time if we did not do it for the whole time window
    if (current - m_min_offset_reset > m_window)
    {
        //std::cerr << "offsetting m_last by " << m_min_offset << std::endl;
        m_last += m_min_offset;
        m_min_offset = period;
        m_min_offset_reset = current;
    }

    // Check for lost samples
    int sample_distance = round((current - m_last) / period);
    if (sample_distance > 1)
        m_lost.push_back(sample_distance - 1);
    else
        m_lost.clear();

    if (static_cast<int>(m_lost.size()) > m_lost_threshold)
    {
        int lost_count = *std::min(m_lost.begin(), m_lost.end());
        m_lost_total += lost_count;
        m_lost.clear();
        //std::cout << "lost " << lost_count << " samples" << std::endl;
        //std::cout << "  original current-last ==" << current - m_last << std::endl;
        //std::cout << "  period ==" << period << std::endl;
        m_last = m_last + period * lost_count;
        //std::cout << "  updated current-last ==" << current - m_last << std::endl;
    }

    if (m_last + period > current)
    {
        //std::cerr << "found earliest sample at " << current-m_last << std::endl;
        m_last = current;
        m_lost.clear();
        m_min_offset = period;
        m_min_offset_reset = current;
    }
    else
    {
        m_last = m_last + period;
        if (current - m_last < 0)
            throw std::logic_error("base time is after current sample");
        m_min_offset = std::min(m_min_offset, current - m_last);
    }

    return base::Time::fromSeconds(m_last);
}


