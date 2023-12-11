#pragma once

#include "kisCore.h"
#include "kisMem.h"


template<typename T>
class kisArray
{
public:
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class Iterator
    {
        friend class kisArray<T>;

    public:
        Iterator(kisArray<T>& array) :
            m_array(array)
        {
        }

        bool operator!=(const Iterator& other)
        {
            return &m_array != &other.m_array || m_pos != other.m_pos;
        }

        T& operator*()
        {
            return m_array[m_pos];
        }

        const Iterator& operator++()
        {
            m_pos++;
            return *this;
        }

    private:
        kisArray<T>& m_array;
        uint32_t m_pos = 0;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class ConstIterator
    {
        friend class kisArray<T>;

    public:
        ConstIterator(const kisArray<T>& array) :
            m_array(array)
        {
        }

        bool operator!=(const ConstIterator& other)
        {
            return &m_array != &other.m_array || m_pos != other.m_pos;
        }

        const T& operator*() const
        {
            return m_array[m_pos];
        }

        const ConstIterator& operator++()
        {
            m_pos++;
            return *this;
        }

    private:
        const kisArray<T>& m_array;
        uint32_t m_pos = 0;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    kisArray(uint32_t reserve = 32)
    {
        m_elements = (T*)kisMemAlloc(sizeof(T) * reserve);
        m_capacity = reserve;
        m_nElements = 0;
    }

    ~kisArray()
    {
        empty();
        kisMemFree(m_elements);
    }

    void add(const T& elem)
    {
        fit(m_nElements + 1);
        m_elements[m_nElements] = elem;
        m_nElements++;
    }

    T& add()
    {
        fit(m_nElements + 1);
        m_nElements++;
        return m_elements[m_nElements - 1];
    }

    void empty()
    {
        for(int i = 0; i < m_nElements; i++)
        {
            m_elements[i].~T();
        }

        m_nElements = 0;
    }

    void removeUnordered(uint32_t index)
    {
        KIS_ASSERT(index < m_nElements);
        (m_elements + index)->~T();
        if(index < m_nElements - 1)
        {
            m_elements[index] = m_elements[m_nElements - 1];
        }
        m_nElements--;
    }

    T* dataPointer()
    {
        return m_elements;
    }

    uint32_t num() const
    {
        return m_nElements;
    }

    uint32_t capacity() const
    {
        return m_capacity;
    }

    T& operator[](uint32_t index)
    {
        return m_elements[index];
    }

    const T& operator[](uint32_t index) const
    {
        return m_elements[index];
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

    void fit(uint32_t num)
    {
        uint32_t newCapacity = kisMax((uint32_t)1, m_capacity);
        while(num >= newCapacity)
        {
            newCapacity *= 2;
        }

        if(newCapacity > m_capacity)
        {
            T* newElements = (T*)kisMemAlloc(newCapacity * sizeof(T));
            for(uint32_t i = 0; i < m_nElements; i++)
            {
                newElements[i] = m_elements[i];
            }

            kisMemFree(m_elements);
            m_elements = newElements;
            m_capacity = newCapacity;
        }
    }

    T* m_elements;
    uint32_t m_nElements;
    uint32_t m_capacity;
};
