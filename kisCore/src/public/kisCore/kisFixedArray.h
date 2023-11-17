#pragma once

#include "kisCore.h"


template<typename T, size_t MaxCount>
class kisFixedArray
{
public:
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class Iterator
    {
        friend class kisFixedArray<T, MaxCount>;

    public:
        Iterator(kisFixedArray<T, MaxCount>& array) :
            m_array(array)
        {
        }

        bool operator!=(const Iterator& other)
        {
            return &m_array != &other.m_array || m_pos != other.m_pos;
        }

        T& operator*()
        {
            KIS_ASSERT(m_pos < m_array.num());
            return m_array[m_pos];
        }

        Iterator& operator++()
        {
            if(m_pos + 1 <= m_array.num())
            {
                m_pos++;
            }

            return *this;
        }

    private:
        kisFixedArray<T, MaxCount>& m_array;
        uint32_t m_pos = 0;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class ConstIterator
    {
        friend class kisFixedArray<T, MaxCount>;
        
    public:
        ConstIterator(const kisFixedArray<T, MaxCount>& array) :
            m_array(array)
        {
        }

        bool operator!=(const ConstIterator& other)
        {
            return &m_array != &other.m_array || m_pos != other.m_pos;
        }

        const T& operator*() const
        {
            KIS_ASSERT(m_pos < m_array.num());
            return m_array[m_pos];
        }

        const ConstIterator& operator++()
        {
            m_pos++;
            return *this;
        }

    private:
        const kisFixedArray<T, MaxCount>& m_array;
        uint32_t m_pos = 0;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void add(const T& elem)
    {
        KIS_ASSERT(m_nElements < MaxCount);
        getElement(m_nElements) = elem;
        m_nElements++;
    }

    void removeUnordered(uint32_t index)
    {
        KIS_ASSERT(index < m_nElements);
        if(index < m_nElements - 1)
        {
            getElement(index) = getElement(m_nElements - 1);
        }

        m_nElements--;
    }

    void clear()
    {
        for(uint32_t i = 0; i < m_nElements; ++i)
        {
            getElement(i).~T();
        }

        m_nElements = 0;
    }

    T* dataPointer()
    {
        return &getElement(0);
    }

    uint32_t* numExternal()
    {
        m_nElements = MaxCount;
        return &m_nElements;
    }

    uint32_t num() const
    {
        return m_nElements;
    }

    uint32_t capacity() const
    {
        return MaxCount;
    }

    T& operator[](uint32_t index)
    {
        return getElement(index);
    }

    const T& operator[](uint32_t index) const
    {
        return getElement(index);
    }

    Iterator begin()
    {
        Iterator beginItr(*this);
        beginItr.m_pos = 0;
        return beginItr;
    }

    Iterator end()
    {
        Iterator endItr(*this);
        endItr.m_pos = num();
        return endItr;
    }

    ConstIterator begin() const
    {
        ConstIterator beginItr(*this);
        beginItr.m_pos = 0;
        return beginItr;
    }

    ConstIterator end() const
    {
        ConstIterator endItr(*this);
        endItr.m_pos = num();
        return endItr;
    }

private:
    T& getElement(uint32_t index)
    {
        return *((T*)(m_data + (index * sizeof(T))));
    }

    uint8_t m_data[sizeof(T) * MaxCount];
    uint32_t m_nElements = 0;
};
