#ifndef _AGGREGATOR_DETERMINE_SAMPLE_TIMESTAMP_HPP_
#define _AGGREGATOR_DETERMINE_SAMPLE_TIMESTAMP_HPP_

#include <base/Time.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>

namespace aggregator
{

/**
 * These methods are used to lookup the timestamp of a sample type.
 * It is assumed that these types have a public field of type base::Time
 * named time, which is the case for all base::samples::* types.
 * For types where this assumption is not true, it is possible to specialized
 * the method in the same namespace which provides a timestamp for this type.
 * E.g.:
 * namespace aggregator {
 *      inline base::Time determineTimestamp(const some_namespace::SomeSampleType& type) {...}
 * }
 */

template<typename T>
inline base::Time determineTimestamp(const T& type)
{
    return type.time;
}

template<typename T>
inline base::Time determineTimestamp(const T* type)
{
    if (type == NULL) throw std::invalid_argument("determineTimestamp: Received null pointer in function!");
    return determineTimestamp(*type);
}

template<typename T>
inline base::Time determineTimestamp(const boost::shared_ptr<T>& type)
{
    if (type.get() == NULL) throw std::invalid_argument("determineTimestamp: Received null pointer in function!");
    return determineTimestamp(*type);
}

}

#endif