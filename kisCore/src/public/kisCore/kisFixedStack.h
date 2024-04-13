#pragma once

#include "kisCoreShared.h"

#include <utility>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
class kisFixedStack
{
public:
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    class Iterator
    {
        friend class KisStack;

    public:
        Iterator(kisFixedStack& stack);
        Iterator& operator++();
        T& get();
        bool isValid() const;

    private:
        kisFixedStack& m_stack;
        int m_index = -1;
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    template<typename ... Args>
    T& push(Args&&... args);

    void pop();

private:
    T m_elements[Capacity];
    uint32_t m_nElements = 0;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
template<typename ...Args>
T& kisFixedStack<T, Capacity>::push(Args&&... args)
{
    KIS_ASSERT(m_nElements < Capacity);
    new(m_elements + m_nElements) T(std::forward<Args>(args)...);
    m_nElements++;
    return m_elements[m_nElements - 1];
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
void kisFixedStack<T, Capacity>::pop()
{
    KIS_ASSERT(m_nElements > 0);
    m_nElements--;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
kisFixedStack<T, Capacity>::Iterator::Iterator(kisFixedStack& stack) :
    m_stack(stack)
{
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
typename kisFixedStack<T, Capacity>::Iterator& kisFixedStack<T, Capacity>::Iterator::operator++()
{
    m_index++;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
T& kisFixedStack<T, Capacity>::Iterator::get()
{
    KIS_ASSERT(isValid());
    return m_stack[m_index];
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, uint32_t Capacity>
bool kisFixedStack<T, Capacity>::Iterator::isValid() const
{
    return m_index < (int)m_stack.m_nElements;
}


