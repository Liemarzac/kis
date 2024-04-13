#pragma once

#include "kisCoreShared.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
class kisFixedHashMap
{
public:
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class Iterator
    {
        friend class kisFixedHashMap<Key, Value, Capacity, HashFunc>;
    public:
        Iterator(kisFixedHashMap& hashMap);
        Iterator& operator++();
        const Key& getKey() const;
        Value& getValue() const;
        bool isValid() const;

    private:
        int m_iEntry;
        kisFixedHashMap& m_hashMap;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    kisFixedHashMap();

    template<typename... Args>
    Iterator add(Key key, Args&&...);
    void remove(Key key);
    void remove(const Iterator& itr);
    Iterator find(Key key);
    Iterator begin();
    uint32_t getNumKeys() const;
    float getCollisionRatio() const;

private:
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    struct Entry
    {
        Key m_key;
        int m_iPrevInList;
        int m_iNextInList;
        int m_iNextInBucket;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    static const int ms_nEntries = Capacity + 2;
    int getBucketIndex(Key key);
    int acquireEntry();
    void releaseEntry(int iEntry);

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    static const int m_iSentinelFreeList = 0;
    static const int m_iSentinelUsedList = 1;
    Entry m_entries[ms_nEntries];
    int m_buckets[ms_nEntries];
    Value m_values[Capacity];
    uint32_t m_nKeys = 0;
    uint32_t m_nCollisions = 0;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
kisFixedHashMap<Key, Value, Capacity, HashFunc>::kisFixedHashMap()
{
    m_entries[m_iSentinelFreeList].m_iPrevInList = ms_nEntries - 1;
    m_entries[m_iSentinelFreeList].m_iNextInList = 2;
    m_entries[m_iSentinelUsedList].m_iPrevInList = m_iSentinelUsedList;
    m_entries[m_iSentinelUsedList].m_iNextInList = m_iSentinelUsedList;

    m_entries[2].m_iPrevInList = m_iSentinelFreeList;
    m_entries[2].m_iNextInList = 3;
    
    for(int i = 3; i < ms_nEntries - 1; i++)
    {
        m_entries[i].m_iPrevInList = i - 1;
        m_entries[i].m_iNextInList = i + 1;
    }

    m_entries[ms_nEntries - 1].m_iPrevInList = ms_nEntries - 2;
    m_entries[ms_nEntries - 1].m_iNextInList = m_iSentinelFreeList;

    for(int i = 0; i < ms_nEntries; i++)
    {
        m_buckets[i] = -1;
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
template<typename... Args>
typename kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator kisFixedHashMap<Key, Value, Capacity, HashFunc>::add(Key key, Args&&... args)
{
    int iBucket = getBucketIndex(key);
    int iEntry = m_buckets[iBucket];
    int iLastEntry = -1;

    while(iEntry != -1)
    {
#if defined(KIS_DEBUG)
        if(m_entries[iEntry].m_key == key)
        {
            // The key already exists.
            KIS_ASSERT(false);
            break;
        }
#endif

        iLastEntry = iEntry;
        iEntry = m_entries[iEntry].m_iNextInBucket;
    }

    if(iEntry == -1)
    {
        iEntry = acquireEntry();

        if(iLastEntry == -1)
        {
            m_buckets[iBucket] = iEntry; // Add as single entry in bucket.
        }
        else
        {
            m_entries[iLastEntry].m_iNextInBucket = iEntry; // Append entry at the end of the bucket.

            m_nCollisions++;
        }

        m_entries[iEntry].m_key = key;
        m_entries[iEntry].m_iNextInBucket = -1;

        m_nKeys++;
    }

    Iterator itr(*this);
    itr.m_iEntry = iEntry;
    return itr;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
void kisFixedHashMap<Key, Value, Capacity, HashFunc>::remove(Key key)
{
    int iBucket = getBucketIndex(key);
    int iEntry = m_buckets[iBucket];
    int iLastEntry = -1;

    while(iEntry != -1)
    {
        if(m_entries[iEntry].m_key == key)
        {
            // Re-link entries in the bucket.
            if(iLastEntry == -1)
            {
                m_buckets[iBucket] = -1;
            }
            else
            {
                m_entries[iLastEntry].m_iNextInBucket = m_entries[iEntry].m_iNextInBucket;
            }
            releaseEntry(iEntry);

            // If the bucket still has keys, it means there were collisions and we have remove one.
            if(m_buckets[iBucket] != -1)
            {
                KIS_ASSERT(m_nCollisions > 0);
                m_nCollisions--;
            }

            KIS_ASSERT(m_nKeys > 0);
            m_nKeys--;

            break;
        }

        iLastEntry = iEntry;
        iEntry = m_entries[iEntry].m_iNextInBucket;
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
void kisFixedHashMap<Key, Value, Capacity, HashFunc>::remove(const Iterator& itr)
{
    KIS_ASSERT(itr.isValid());
    Key key = m_entries[itr.m_iEntry].m_key;
    remove(key);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
typename kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator kisFixedHashMap<Key, Value, Capacity, HashFunc>::find(Key key)
{
    int iBucket = getBucketIndex(key);
    int iEntry = m_buckets[iBucket];
    int iPrevEntry = -1;

    while(iEntry != -1)
    {
        if(m_entries[iEntry].m_key == key)
        {
            break;
        }

        iPrevEntry = iEntry;
        iEntry = m_entries[iEntry].m_iNextInBucket;
    }

    Iterator itr(*this);
    itr.m_iEntry = iEntry;
    return itr;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
typename kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator kisFixedHashMap<Key, Value, Capacity, HashFunc>::begin()
{
    Iterator itr(*this);
    itr.m_iEntry = m_entries[m_iSentinelUsedList].m_iNextInList;
    return itr;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
uint32_t kisFixedHashMap<Key, Value, Capacity, HashFunc>::getNumKeys() const
{
    return m_nKeys;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
float kisFixedHashMap<Key, Value, Capacity, HashFunc>::getCollisionRatio() const
{
    if(m_nKeys == 0)
    {
        return 0.0f;
    }

    return (float)m_nCollisions / (float)m_nKeys;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
int kisFixedHashMap<Key, Value, Capacity, HashFunc>::acquireEntry()
{
    int iAcquired = m_entries[m_iSentinelFreeList].m_iNextInList;
    KIS_ASSERT(iAcquired != m_iSentinelFreeList); // Full if this assert triggers.

    // Remove the acquired entry from the free list.
    m_entries[m_iSentinelFreeList].m_iNextInList = m_entries[iAcquired].m_iNextInList;
    m_entries[m_entries[iAcquired].m_iNextInList].m_iPrevInList = m_iSentinelFreeList;

    // Add the acquired entry in the used list
    int iInsertAfter = m_entries[m_iSentinelUsedList].m_iPrevInList;
    m_entries[iInsertAfter].m_iNextInList = iAcquired;
    m_entries[m_iSentinelUsedList].m_iPrevInList = iAcquired;

    // Re-link acquired entry in the used list.
    m_entries[iAcquired].m_iPrevInList = iInsertAfter;
    m_entries[iAcquired].m_iNextInList = m_iSentinelUsedList;
    
    return iAcquired;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
void kisFixedHashMap<Key, Value, Capacity, HashFunc>::releaseEntry(int iEntry)
{
    // Remove the entry from the used list.
    m_entries[m_entries[iEntry].m_iPrevInList].m_iNextInList = m_entries[iEntry].m_iNextInList;
    m_entries[m_entries[iEntry].m_iNextInList].m_iPrevInList = m_entries[iEntry].m_iPrevInList;;

    // Add the acquired entry in the free list
    int iInsertBefore = m_entries[m_iSentinelFreeList].m_iNextInList;
    m_entries[iInsertBefore].m_iPrevInList = iEntry;
    m_entries[m_iSentinelFreeList].m_iNextInList = iEntry;

    // Re-link acquired entry in the used list.
    m_entries[iEntry].m_iPrevInList = m_iSentinelFreeList;
    m_entries[iEntry].m_iNextInList = iInsertBefore;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
int kisFixedHashMap<Key, Value, Capacity, HashFunc>::getBucketIndex(Key key)
{
    int iBucket = HashFunc(key);
    if(iBucket >= Capacity)
    {
        iBucket %= Capacity;
    }

    return iBucket + 2;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator::Iterator(kisFixedHashMap& hashMap) :
    m_hashMap(hashMap)
{
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
typename kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator& kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator::operator++()
{
    KIS_ASSERT(isValid());
    m_iEntry = m_hashMap.m_entries[m_iEntry].m_iNextInList;
    return *this;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
const Key& kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator::getKey() const
{
    KIS_ASSERT(isValid());
    return m_hashMap.m_entries[m_iEntry].m_key;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
Value& kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator::getValue() const
{
    KIS_ASSERT(isValid());
    const int iValue = m_iEntry - 2;
    KIS_ASSERT(iValue >= 0 && iValue < Capacity);
    return m_hashMap.m_values[iValue];
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Key, typename Value, uint32_t Capacity, uint32_t (*HashFunc)(Key)>
bool kisFixedHashMap<Key, Value, Capacity, HashFunc>::Iterator::isValid() const
{
    return m_iEntry != kisFixedHashMap::m_iSentinelUsedList;
}
