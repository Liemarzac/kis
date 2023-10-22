#pragma once

#include "kisCore.h"

enum class kisArrayStaticInit
{
    Empty,
    Fill
};

template<typename T, size_t MaxCount>
class kisArrayStaticIterator;

template<typename T, size_t MaxCount>
class kisArrayStaticConstIterator;

template<typename T, size_t MaxCount>
class kisArrayStatic
{
public:
    kisArrayStatic(kisArrayStaticInit init = kisArrayStaticInit::Empty)
    {
        m_nElements = init == kisArrayStaticInit::Empty ? 0 : MaxCount;
    }

    void add(const T& elem)
    {
        KIS_ASSERT(m_nElements < MaxCount);
        m_elements[m_nElements] = elem;
        m_nElements++;
    }
    
    void removeUnordered(uint32_t index)
    {
        KIS_ASSERT(index < m_nElements);
        if(index < m_nElements - 1)
        {
            m_elements[index] = m_elements[m_nElements - 1];
        }

        m_nElements--;
    }

    void fill()
    {
        m_nElements = MaxCount;
    }

    void empty()
    {
        for(uint32_t i = 0; i < m_nElements; ++i)
        {
            (m_elements + i)->~T();
        }

        m_nElements = 0;
    }

    T* dataPointer()
    {
        return m_elements;
    }
    
    uint32_t* numPointer()
    {
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
        return m_elements[index];
    }

    const T& operator[](uint32_t index) const
    {
        return m_elements[index];
    }

    kisArrayStaticIterator<T, MaxCount> begin()
    {
        kisArrayStaticIterator<T, MaxCount> beginItr(*this);
        beginItr.m_pos = 0;
        return beginItr;
    }

    kisArrayStaticIterator<T, MaxCount> end()
    {
        kisArrayStaticIterator<T, MaxCount> endItr(*this);
        endItr.m_pos = num();
        return endItr;
    }

    kisArrayStaticConstIterator<T, MaxCount> begin() const
    {
        kisArrayStaticConstIterator<T, MaxCount> beginItr(*this);
        beginItr.m_pos = 0;
        return beginItr;
    }

    kisArrayStaticConstIterator<T, MaxCount> end() const
    {
        kisArrayStaticConstIterator<T, MaxCount> endItr(*this);
        endItr.m_pos = num();
        return endItr;
    }

private:
    T m_elements[MaxCount];
    uint32_t m_nElements;
};

template<typename T, size_t MaxCount>
class kisArrayStaticIterator
{
    friend class kisArrayStatic<T, MaxCount>;

public:
    kisArrayStaticIterator(kisArrayStatic<T, MaxCount>& array) :
        m_array(array)
    {
    }

    bool operator!=(const kisArrayStaticIterator& other)
    {
        return &m_array != &other.m_array || m_pos != other.m_pos;
    }

    T& operator*()
    {
        return m_array[m_pos];
    }

    const kisArrayStaticIterator& operator++()
    {
        m_pos++;
        return *this;
    }

private:
    kisArrayStatic<T, MaxCount>& m_array;
    uint32_t m_pos = 0;
};

template<typename T, size_t MaxCount>
class kisArrayStaticConstIterator
{
    friend class kisArrayStatic<T, MaxCount>;
    
public:
    kisArrayStaticConstIterator(const kisArrayStatic<T, MaxCount>& array) :
        m_array(array)
    {
    }

    bool operator!=(const kisArrayStaticConstIterator& other)
    {
        return &m_array != &other.m_array || m_pos != other.m_pos;
    }

    const T& operator*() const
    {
        return m_array[m_pos];
    }

    const kisArrayStaticConstIterator& operator++()
    {
        m_pos++;
        return *this;
    }

private:
    const kisArrayStatic<T, MaxCount>& m_array;
    uint32_t m_pos = 0;
};
