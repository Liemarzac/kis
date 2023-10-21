#pragma once

#include "core.h"

enum class ArrayStatic_Init
{
    Empty,
    Fill
};

template<typename T, size_t MaxCount>
class ArrayStaticIterator;

template<typename T, size_t MaxCount>
class ArrayStaticConstIterator;

template<typename T, size_t MaxCount>
class ArrayStatic
{
public:
    ArrayStatic(ArrayStatic_Init init = ArrayStatic_Init::Empty)
    {
        m_nElements = init == ArrayStatic_Init::Empty ? 0 : MaxCount;
    }

    void add(const T& elem)
    {
        ASSERT(m_nElements < MaxCount);
        m_elements[m_nElements] = elem;
        m_nElements++;
    }
    
    void removeUnordered(uint32_t index)
    {
        ASSERT(index < m_nElements);
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

    ArrayStaticIterator<T, MaxCount> begin()
    {
        ArrayStaticIterator<T, MaxCount> beginItr(*this);
        beginItr.m_pos = 0;
        return beginItr;
    }

    ArrayStaticIterator<T, MaxCount> end()
    {
        ArrayStaticIterator<T, MaxCount> endItr(*this);
        endItr.m_pos = num();
        return endItr;
    }

    ArrayStaticConstIterator<T, MaxCount> begin() const
    {
        ArrayStaticConstIterator<T, MaxCount> beginItr(*this);
        beginItr.m_pos = 0;
        return beginItr;
    }

    ArrayStaticConstIterator<T, MaxCount> end() const
    {
        ArrayStaticConstIterator<T, MaxCount> endItr(*this);
        endItr.m_pos = num();
        return endItr;
    }

private:
    T m_elements[MaxCount];
    uint32_t m_nElements;
};

template<typename T, size_t MaxCount>
class ArrayStaticIterator
{
    friend class ArrayStatic<T, MaxCount>;

public:
    ArrayStaticIterator(ArrayStatic<T, MaxCount>& array) :
        m_array(array)
    {
    }

    bool operator!=(const ArrayStaticIterator& other)
    {
        return &m_array != &other.m_array || m_pos != other.m_pos;
    }

    T& operator*()
    {
        return m_array[m_pos];
    }

    const ArrayStaticIterator& operator++()
    {
        m_pos++;
        return *this;
    }

private:
    ArrayStatic<T, MaxCount>& m_array;
    uint32_t m_pos = 0;
};

template<typename T, size_t MaxCount>
class ArrayStaticConstIterator
{
    friend class ArrayStatic<T, MaxCount>;
    
public:
    ArrayStaticConstIterator(const ArrayStatic<T, MaxCount>& array) :
        m_array(array)
    {
    }

    bool operator!=(const ArrayStaticConstIterator& other)
    {
        return &m_array != &other.m_array || m_pos != other.m_pos;
    }

    const T& operator*() const
    {
        return m_array[m_pos];
    }

    const ArrayStaticConstIterator& operator++()
    {
        m_pos++;
        return *this;
    }

private:
    const ArrayStatic<T, MaxCount>& m_array;
    uint32_t m_pos = 0;
};
