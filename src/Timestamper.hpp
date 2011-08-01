#ifndef AGGREGATOR_TIMESTAMP_SYNCHRONIZER_HPP
#define AGGREGATOR_TIMESTAMP_SYNCHRONIZER_HPP

#include <base/time.h>
#include <list>
#include <TimestampEstimator.hpp>

namespace aggregator
{
    /** This class handles most list item splicing:
     *  popItem:      m_synchItems  => m_spareItems
     *  getSpareItem: m_spareItems  => m_sharedItems
     *  putSpareItem: m_sharedItems => m_spareItems
     *  pushItem:     m_sharedItems => m_Items
     *
     * The obvious hole is m_Items => m_synchItems, which must be
     * handled by the derived class.
     */
    template<typename Item>
    class TimestamperBase
    {
    public:
	struct ItemInfo
	{
	    Item item;
	    base::Time time;
	};
	typedef std::list<ItemInfo> ItemList;
	typedef typename ItemList::iterator ItemIterator;
    protected:
	ItemList m_items;
	ItemList m_synchItems;
	ItemList m_spareItems;
	ItemList m_sharedItems;
    public:
	/** Push an initialized ItemInfo. The iterator must be obtained from
	 *  getSpareItem
	 */
	void pushItem(ItemIterator const &item);

	/** Check whether there are Items that can be fetched using \c item()
	 *  or discared with popItem()
	 */
	bool itemAvailable();

	/** Return a reference to the oldest processed item
	 */
	ItemInfo &item();

	/** Discard the oldest processed item
	 */
	void popItem();

	/** Retrieve an iterator to an unused ItemInfo
	 */
	ItemIterator getSpareItem();

	/** Give an iterator to an ItemInfo back. The iterator must be obtained
	 *  from getSpareItem.
	 */
	void putSpareItem(ItemIterator const &item);
    };


    template<typename Item>
    bool TimestamperBase<Item>::itemAvailable()
    {
	return !m_synchItems.empty();
    }

    template<typename Item>
    typename TimestamperBase<Item>::ItemInfo &TimestamperBase<Item>::item()
    {
	return m_synchItems.front();
    }

    template<typename Item>
    void TimestamperBase<Item>::popItem()
    {
	m_spareItems.splice(m_spareItems.end(),
			    m_synchItems,
			    m_synchItems.begin());
    }

    template<typename Item>
    typename TimestamperBase<Item>::ItemIterator TimestamperBase<Item>::getSpareItem()
    {
	ItemIterator it;
	if(!m_spareItems.empty())
	{
	    m_sharedItems.splice(m_sharedItems.begin(),
				 m_spareItems,
				 m_spareItems.begin());
	}
	else
	    m_sharedItems.push_front(ItemInfo());
	return m_sharedItems.begin();
    }

    template<typename Item>
    void TimestamperBase<Item>::pushItem(ItemIterator const &item)
    {
	m_items.splice(m_items.end(),
		       m_sharedItems,
		       item);
    }

    template<typename Item>
    void TimestamperBase<Item>::putSpareItem(ItemIterator const &item)
    {
	m_spareItems.splice(m_spareItems.begin(),
			     m_sharedItems,
			     item);
    }

    template<typename Item>
    class Timestamper : public TimestamperBase<Item>
    {
    public:
	typedef typename TimestamperBase<Item>::ItemInfo ItemInfo;
	typedef typename TimestamperBase<Item>::ItemList ItemList;
	typedef typename TimestamperBase<Item>::ItemIterator ItemIterator;
    private:
	std::list<base::Time> m_refs;
	base::Time m_maxItemLatency;
	base::Time m_matchWindowOldest;
	base::Time m_matchWindowNewest;
	bool m_useRefEstimator;
	unsigned int m_last_item_ctr;
	unsigned int m_last_ref_ctr;
	bool m_have_item_ctr;
	bool m_have_ref_ctr;
	bool m_doSynchronize;

	TimestampEstimator refTsEstimator;
	TimestampEstimator itemTsEstimator;

	void synchronizeItems(base::Time const & now);
    public:
	/** Constructs a Timestamper
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
	Timestamper(base::Time const & maxItemLatency,
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

	/** Push an initialized ItemInfo. The iterator must be obtained from
	 *  getSpareItem
	 */
	void pushItem(ItemIterator const &item);

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

	/** Check whether there are Items that can be fetched using \c item()
	 *  or discared with popItem()
	 */
	bool itemAvailable(base::Time const & now);

	/** Synchronize the Tiemstamp time. Returns true on success.
	 *  This only succeeds if there is a matching reference and no
	 *  valid items in the synchronizer.
	 */
	bool getTimeFor(base::Time &time);

	/** Enabled the synchronization. Should be called when
	 *  references become available.
	 */
	void enableSynchronization();
    };

    template<typename Item>
    Timestamper<Item>::Timestamper
    (base::Time const & maxItemLatency,
     base::Time const & matchWindowOldest,
     base::Time const & matchWindowNewest,
     base::Time const & estimatorWindow,
     base::Time const & estimatorInitialPeriod,
     int estimatorLostThreshold)
	: m_maxItemLatency(maxItemLatency)
	, m_matchWindowOldest(matchWindowOldest)
	, m_matchWindowNewest(matchWindowNewest)
	, m_useRefEstimator(estimatorWindow != base::Time::fromMicroseconds(0))
	, m_have_item_ctr(false)
	, m_have_ref_ctr(false)
	, m_doSynchronize(false)
	, refTsEstimator(estimatorWindow,
			 estimatorInitialPeriod,
			 base::Time::fromMicroseconds(0),
			 estimatorLostThreshold)
	, itemTsEstimator(estimatorWindow,
			  estimatorInitialPeriod,
			  base::Time::fromMicroseconds(0),
			  estimatorLostThreshold)
    {
    }

    template<typename Item>
    void Timestamper<Item>::pushItem(Item const &item, base::Time const & time)
    {
	ItemIterator it = TimestamperBase<Item>::getSpareItem();

	it->item = item;
	it->time = time;
	pushItem(it);
    }

    template<typename Item>
    void Timestamper<Item>::pushItem(Item const &item, base::Time const & time,
						    unsigned int ctr)
    {
      if (m_have_item_ctr && ctr - m_last_item_ctr > 1 &&
	  (ctr > m_last_item_ctr || ctr - m_last_item_ctr < 1000))
	    lostItems(ctr - m_last_item_ctr);
	m_last_item_ctr = ctr;
	m_have_item_ctr = true;
	pushItem(item,time);
    }

    template<typename Item>
    void Timestamper<Item>::pushItem(ItemIterator const &item)
    {
	TimestamperBase<Item>::pushItem(item);
    }

    template<typename Item>
    void Timestamper<Item>::lostItems(unsigned int count)
    {
	if (!m_doSynchronize) {
	    for(;count > 0; count--)
		itemTsEstimator.updateLoss();
	}
    }

    template<typename Item>
    void Timestamper<Item>::pushReference(base::Time const & ref)
    {
	m_doSynchronize = true;
	//cascading a TimestampEstimator here gives a nicer estimate
	m_refs.push_back(ref);

	synchronizeItems(ref);
    }

    template<typename Item>
    void Timestamper<Item>::pushReference(base::Time const & ref,
						    unsigned int ctr)
    {
	if (m_have_ref_ctr && ctr - m_last_ref_ctr > 1 &&
	    (ctr > m_last_ref_ctr || ctr - m_last_ref_ctr < 1000))
	    lostReferences(ctr - m_last_ref_ctr);
	m_last_ref_ctr = ctr;
	m_have_ref_ctr = true;
	pushReference(ref);
    }

    template<typename Item>
    void Timestamper<Item>::lostReferences(unsigned int count)
    {
	//we should probably tell the timestamp estimator about this
    }

    template<typename Item>
    bool Timestamper<Item>::fetchItem(Item &item,
						base::Time & time,
						base::Time const & now)
    {
	if (!itemAvailable(now))
	    return false;
	item = this->item().item;
	time = this->item().time;
	TimestamperBase<Item>::popItem();
	return true;
    }

    template<typename Item>
    bool Timestamper<Item>::itemAvailable(base::Time const & now)
    {
	synchronizeItems(now);
	return TimestamperBase<Item>::itemAvailable();
    }

    template<typename Item>
    void Timestamper<Item>::synchronizeItems(base::Time const & now)
    {
	ItemIterator item_front = TimestamperBase<Item>::m_items.begin();

	if (m_doSynchronize)
	{
	    typename std::list<base::Time>::iterator refs_front = m_refs.begin();

	    //drop TS in m_refs that are before the oldest m_items TS minus
	    //maximum latency for matching the item.
	    while(refs_front != m_refs.end() &&
		  item_front != TimestamperBase<Item>::m_items.end() &&
		  *refs_front + m_matchWindowOldest < item_front->time)
	    {
		if (m_useRefEstimator)
		    refTsEstimator.update(*refs_front);

		if (*refs_front + m_matchWindowNewest > item_front->time)
		{
		    //got a match
		    item_front->time = m_refs.front();
		    item_front++;
		}

		refs_front++;
	    }

	    //finally, send all m_items that sit in our buffer and are too old on their way(with the guessed timestamp)
	    while(item_front != TimestamperBase<Item>::m_items.end() &&
		  (item_front->time < now - m_maxItemLatency ||
		   (refs_front != m_refs.end() &&
		    *refs_front + m_matchWindowOldest >= item_front->time)))
	    {
		if (m_useRefEstimator)
		{
		    if(refTsEstimator.haveEstimate())
			item_front->time = refTsEstimator.updateLoss();
		    else
			refTsEstimator.updateLoss();
		    refTsEstimator.shortenSampleList(item_front->time);
		}

		item_front++;
	    }

	    m_refs.erase(m_refs.begin(), refs_front);
	}
	else
	{
	    while(item_front != TimestamperBase<Item>::m_items.end())
	    {
		item_front->time = itemTsEstimator.update(item_front->time);
		item_front++;
	    }
	}

	//std::list::splice is constant time
	TimestamperBase<Item>::m_synchItems.splice(
	    TimestamperBase<Item>::m_synchItems.end(),
	    TimestamperBase<Item>::m_items,
	    TimestamperBase<Item>::m_items.begin(),
	    item_front);
    }

    template<typename Item>
    bool Timestamper<Item>::getTimeFor(base::Time &time)
    {
	//there is already another item in the queue, need to use the slow path
	if (!TimestamperBase<Item>::m_synchItems.empty() ||
	    !TimestamperBase<Item>::m_items.empty())
	    return false;

	//what follows essentially is synchronizeItems adjusted for
	//m_items.empty() => true, m_items.front().time => time, now => time,
	//m_synchItems.front().time => time, m_synchItems.front().item ignored

	//drop TS in m_refs that are before the oldest m_items TS minus
	//maximum latency for matching the item.
	if (m_doSynchronize)
	{
	    while(!m_refs.empty() && m_refs.front() + m_matchWindowOldest < time)
	    {
		if (m_useRefEstimator)
		    refTsEstimator.update(m_refs.front());

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
	else
	{
	    time = itemTsEstimator.update(time);
	    return true;
	}
    }

    template<typename Item>
    void Timestamper<Item>::enableSynchronization()
    {
	m_doSynchronize = true;
    }
}

#endif /*AGGREGATOR_TIMESTAMP_SYNCHRONIZER_HPP*/
