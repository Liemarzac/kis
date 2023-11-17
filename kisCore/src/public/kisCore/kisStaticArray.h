#pragma once

#include "kisCore.h"


template<typename T, size_t Count>
class kisStaticArray
{
public:
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class Iterator
    {
        friend class kisArrayStatic;

    public:
        Iterator(kisStaticArray<T, Count>& array) :
            m_array(array)
        {
        }

        bool operator!=(const Iterator& other)
        {
            return &m_array != &other.m_array || m_pos != other.m_pos;
        }

        T& operator*()
        {
            KIS_ASSERT(m_pos < Count);
            return m_array[m_pos];
        }

        const Iterator& operator++()
        {
            if(m_pos + 1 <= Count)
            {
                m_pos++;
            }
            return *this;
        }

    private:
        kisStaticArray<T, Count>& m_array;
        uint32_t m_pos = 0;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class ConstIterator
    {
        friend class kisArrayStatic;

    public:
        ConstIterator(const kisStaticArray<T, Count>& array):
            m_array(array)
        {
        }

        bool operator!=(const ConstIterator& other)
        {
            return &m_array != &other.m_array || m_pos != other.m_pos;
        }

        const T& operator*()
        {
            KIS_ASSERT(m_pos < Count);
            return m_array[m_pos];
        }

        ConstIterator& operator++()
        {
            if(m_pos + 1 <= Count)
            {
                m_pos++;
            }
            return *this;
        }

    private:
        const kisStaticArray<T, Count>& m_array;
        uint32_t m_pos = 0;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    T* dataPointer()
    {
        return m_elements;
    }

    uint32_t num() const
    {
        return Count;
    }

    T& operator[](uint32_t index)
    {
        KIS_ASSERT(index < Count);
        return m_elements[index];
    }

    const T& operator[](uint32_t index) const
    {
        KIS_ASSERT(index < Count);
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
        endItr.m_pos = Count;
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
        endItr.m_pos = Count;
        return endItr;
    }

private:
    T m_elements[Count];
};
