#ifndef AGGREGATOR_TIMESTAMP_SYNCHRONIZER_HPP
#define AGGREGATOR_TIMESTAMP_SYNCHRONIZER_HPP

#include <base/time.h>
#include <list>
#include <TimestampEstimator.hpp>

namespace aggregator
{
    template<typename Item>
    class TimestampSynchronizer
    {
    public:
	struct ItemInfo
	{
	    Item item;
	    base::Time time;
	};
    private:
	std::list<ItemInfo> m_items;
	std::list<ItemInfo> m_synchItems;
	std::list<base::Time> m_refs;
	base::Time m_maxItemLatency;
	base::Time m_matchWindowOldest;
	base::Time m_matchWindowNewest;
	bool m_useEstimator;
	unsigned int m_last_item_ctr;
	unsigned int m_last_ref_ctr;
	bool m_have_item_ctr;
	bool m_have_ref_ctr;

	TimestampEstimator tsestimator;

	void synchronizeItems(base::Time const & now);
    public:
	/** Constructs a TimestampSynchronizer
	 * @param maxItemLatency     Maximum age of items in the internal list
	 * @param matchWindowOldest  The oldest relative item time at which a
	 *                           given reference timestamp matches the item
	 *                           time
	 * @param matchWindowNewest  The newest relative item time at which a
	 *                           given reference timestamp matches the item
	 *                           time
	 * @param estimatorWindow    The window size to use to estimate
	 *                           lost reference timestamps, 0 means not
	 *                           using the estimator at all
	 *                           see TimestampEstimator
	 * @param estimatorInitiailPeriod  The initial period for the estimator
	 *                           see TimestampEstimator
	 * @param estimatorLostThreashold  The lost threshold for the estimator
	 *                           see TimestampEstimator
	 */
	TimestampSynchronizer(base::Time const & maxItemLatency,
			      base::Time const & matchWindowOldest,
			      base::Time const & matchWindowNewest,
			      base::Time const & estimatorWindow,
			      base::Time const & estimatorInitialPeriod
			      = base::Time::fromSeconds(-1),
			      int estimatorLostThreshold = 2);

	/** Push an (item, time) pair into the internal list. */
	void pushItem(Item const &item, base::Time const & time);

	/** Push an (item, time) pair into the internal list
	 *  and push losses according to the item counter */
	void pushItem(Item const &item, base::Time const & time,
		      unsigned int ctr);

	/** Push information about lost items into the internal data
	 * structures */
	void lostItems(unsigned int count);

	/** Push a reference timestamp into the internal list. */
	void pushReference(base::Time const & ref);

	/** Push a reference timestamp into the internal list
	 *  and push losses according to the reference counter */
	void pushReference(base::Time const & ref,
			   unsigned int ctr);

	/** Push information about lost reference timestamps into the internal
	 * data structures */
	void lostReferences(unsigned int count);

	/** Fetch a synchronized item, time pair from the internal lists, using
	 *  now and maxItemLatency to determine lost reference timestamps.
	 */
	bool fetchItem(Item &item, base::Time & time, base::Time const & now);

	//these require some internal changes to be implemented efficiently
	bool itemAvailable(base::Time const & now);

	ItemInfo &item();

	void popItem();

	//this one is 100% speculative, but if it works avoids another copy
	//uses and modifies the time parameter
	//only works if there is a matching reference and the item list is empty
	bool getTimeFor(base::Time &time);
    };

    template<typename Item>
    TimestampSynchronizer<Item>::TimestampSynchronizer
    (base::Time const & maxItemLatency,
     base::Time const & matchWindowOldest,
     base::Time const & matchWindowNewest,
     base::Time const & estimatorWindow,
     base::Time const & estimatorInitialPeriod,
     int estimatorLostThreshold)
	: m_maxItemLatency(maxItemLatency)
	, m_matchWindowOldest(matchWindowOldest)
	, m_matchWindowNewest(matchWindowNewest)
	, m_useEstimator(estimatorWindow != base::Time::fromMicroseconds(0))
	, tsestimator(estimatorWindow,
		      estimatorInitialPeriod,
		      estimatorLostThreshold)
    {
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::pushItem(Item const &item, base::Time const & time)
    {
	m_items.push_back(ItemInfo());
	m_items.back().item = item;
	m_items.back().time = time;
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::pushItem(Item const &item, base::Time const & time,
						    unsigned int ctr)
    {
	if (m_have_item_ctr && ctr - m_last_item_ctr > 1)
	    lostItems(ctr - m_last_item_ctr);
	m_last_item_ctr = ctr;
	pushItem(item,time);
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::lostItems(unsigned int count)
    {
	//we should tell the timestamp estimator about this
	//only really needed when there are no references
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::pushReference(base::Time const & ref)
    {
	//cascading a TimestampEstimator here gives a nicer estimate
	m_refs.push_back(ref);

	synchronizeItems(ref);
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::pushReference(base::Time const & ref,
						    unsigned int ctr)
    {
	if (m_have_ref_ctr && ctr - m_last_ref_ctr > 1)
	    lostReferences(ctr - m_last_ref_ctr);
	m_last_ref_ctr = ctr;
	pushReference(ref);
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::lostReferences(unsigned int count)
    {
	//we should probably tell the timestamp estimator about this
    }

    template<typename Item>
    bool TimestampSynchronizer<Item>::fetchItem(Item &item,
						base::Time & time,
						base::Time const & now)
    {
	if (!itemAvailable(now))
	    return false;
	item = this->item().item;
	time = this->item().time;
	popItem();
	return true;
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::synchronizeItems(base::Time const & now)
    {
	typename std::list<ItemInfo>::iterator item_front = m_items.begin();
	typename std::list<base::Time>::iterator refs_front = m_refs.begin();

	//drop TS in m_refs that are before the oldest m_items TS minus
	//maximum latency for matching the item.
	while(refs_front != m_refs.end() && item_front != m_items.end() &&
	      *refs_front + m_matchWindowOldest < item_front->time)
	{
	    if (m_useEstimator)
		tsestimator.update(*refs_front);

	    if (*refs_front + m_matchWindowNewest > item_front->time)
	    {
		//got a match
		item_front->time = m_refs.front();
		item_front++;
	    }

	    refs_front++;
	}

	//finally, send all m_items that sit in our buffer and are too old on their way(with the guessed timestamp)
	while(item_front != m_items.end() &&
	      (item_front->time < now - m_maxItemLatency ||
	       (refs_front != m_refs.end() &&
		*refs_front + m_matchWindowOldest >= item_front->time)))
	{
	    if (m_useEstimator)
	    {
		if(tsestimator.haveEstimate())
		    item_front->time = tsestimator.updateLoss();
		else
		    tsestimator.updateLoss();
		tsestimator.shortenSampleList(item_front->time);
	    }

	    item_front++;
	}

	//std::list::splice is constant time
	m_synchItems.splice(m_synchItems.end(),
			    m_items,
			    m_items.begin(),
			    item_front);

	m_refs.erase(m_refs.begin(), refs_front);
    }

    template<typename Item>
    bool TimestampSynchronizer<Item>::itemAvailable(base::Time const & now)
    {
	synchronizeItems(now);
	return !m_synchItems.empty();
    }

    template<typename Item>
    typename TimestampSynchronizer<Item>::ItemInfo &TimestampSynchronizer<Item>::item()
    {
	return m_synchItems.front();
    }

    template<typename Item>
    void TimestampSynchronizer<Item>::popItem()
    {
	m_synchItems.pop_front();
    }

    template<typename Item>
    bool TimestampSynchronizer<Item>::getTimeFor(base::Time &time)
    {
	//there is already another item in the queue, need to use the slow path
	if (!m_synchItems.empty() || !m_items.empty())
	    return false;

	//what follows essentially is synchronizeItems adjusted for
	//m_items.empty() => true, m_items.front().time => time, now => time,
	//m_synchItems.front().time => time, m_synchItems.front().item ignored

	//drop TS in m_refs that are before the oldest m_items TS minus
	//maximum latency for matching the item.
	while(!m_refs.empty() && m_refs.front() + m_matchWindowOldest < time)
	{
	    if (m_useEstimator)
		tsestimator.update(m_refs.front());

	    if (m_refs.front() + m_matchWindowNewest > time)
	    {
		//got a match
		time = m_refs.front();
		m_refs.pop_front();
		return true;
	    }

	    m_refs.pop_front();
	}

	return false;
    }
}

#endif /*AGGREGATOR_TIMESTAMP_SYNCHRONIZER_HPP*/
