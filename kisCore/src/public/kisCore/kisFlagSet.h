#pragma once

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType = uint32_t>
class kisFlagSet
{
public:
    kisFlagSet();
    void set(T flag);
    void remove(T flag);
    bool isSet(T flag) const;
    bool isAnySet() const;
    void clear();

private:
    ContainerType m_flags;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType>
kisFlagSet<T, ContainerType>::kisFlagSet() :
    m_flags(0)
{
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType>
void kisFlagSet<T, ContainerType>::set(T flag)
{
    m_flags |= (1 << (uint32_t)flag);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType>
void kisFlagSet<T, ContainerType>::remove(T flag)
{
    m_flags &= ~(1 << (uint32_t)flag);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType>
bool kisFlagSet<T, ContainerType>::isSet(T flag) const
{
    return (m_flags & (1 << (uint32_t)flag)) != 0;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType>
bool kisFlagSet<T, ContainerType>::isAnySet() const
{
    return m_flags != 0;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T, typename ContainerType>
void kisFlagSet<T, ContainerType>::clear()
{
    m_flags = 0;
}
