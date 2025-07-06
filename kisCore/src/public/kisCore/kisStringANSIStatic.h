#pragma once

#include "kisCore.h"

#include <cstdio>
#include <cstring>
#include <stdarg.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
class kisStringANSIStatic;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
typedef kisStringANSIStatic<32> kisString32;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
typedef kisStringANSIStatic<64> kisString64;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
class kisStringANSIStatic
{
public:
    kisStringANSIStatic();
    kisStringANSIStatic(const char* str);

    void concat(const char* format, ...);
    const char* c_str() const;
    uint32_t len() const;
    bool operator==(const char* other) const;
    bool trimLeftOf(const char* str);
    int lastOccurenceOf(const char * str) const;
    bool substring(int from, int to);

    static kisStringANSIStatic format(const char* format, ...);

private:
    char m_buffer[Size];
    uint32_t m_nUsed = 0;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
kisStringANSIStatic<Size>::kisStringANSIStatic()
{
    m_buffer[0] = '\0';
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
kisStringANSIStatic<Size>::kisStringANSIStatic(const char* str)
{
    m_buffer[0] = '\0';
    concat(str);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
void kisStringANSIStatic<Size>::concat(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char* write = m_buffer + m_nUsed;
    char* bufferEnd = m_buffer + Size;
    int64_t nMaxWrite = bufferEnd - write;
    if(nMaxWrite > 1)
    {
        vsnprintf(write, nMaxWrite, format, args);
        size_t nWritten = strlen(write);
        m_nUsed += nWritten;
        m_buffer[m_nUsed] = '\0';
    }

    va_end(args);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
const char* kisStringANSIStatic<Size>::c_str() const
{
    return m_buffer;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
uint32_t kisStringANSIStatic<Size>::len() const
{
    return m_nUsed;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
bool kisStringANSIStatic<Size>::operator==(const char* other) const
{
    return strcmp(m_buffer, other) == 0;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
bool kisStringANSIStatic<Size>::trimLeftOf(const char* str)
{
    const size_t strLen = strlen(str);

    if(strLen > m_nUsed)
    {
        return false;
    }

    for(uint32_t i = 0; i <= m_nUsed - strLen; i++)
    {
        if(memcmp(m_buffer + i, str, strLen) == 0)
        {
            char tmp[Size];
            strcpy(tmp, m_buffer + i);
            strcpy(m_buffer, tmp);
            m_nUsed -= i;
            return true;
        }
    }
    
    return false;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
int kisStringANSIStatic<Size>::lastOccurenceOf(const char * str) const
{
    const size_t strLen = strlen(str);

    if(strLen > m_nUsed)
    {
        return false;
    }

    for(int i = (int)(m_nUsed - strLen); i > 0; i--)
    {
        if(memcmp(m_buffer + i, str, strLen) == 0)
        {
            return i;
        }
    }

    return -1;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
bool kisStringANSIStatic<Size>::substring(int from, int to)
{
    if(from < 0 || from > m_nUsed)
    {
        return false;
    }

    if(to < 0 || to > m_nUsed)
    {
        return false;
    }

    if(from >= to)
    {
        return false;
    }

    const size_t copySize = to - from;

    char tmp[Size];
    memcpy(tmp, m_buffer + from, copySize);
    memcpy(m_buffer, tmp, copySize);
    m_buffer[copySize] = '\0';
    m_nUsed = (uint32_t)copySize;

    return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<size_t Size>
kisStringANSIStatic<Size> kisStringANSIStatic<Size>::format(const char* format, ...)
{
    kisStringANSIStatic str;

    va_list args;
    va_start(args, format);
    int nChars = vsnprintf(NULL, 0, format, args);
    if(nChars + 1 > Size)
    {
        goto error;
    }

    vsnprintf(str.m_buffer, Size, format, args);
    str.m_nUsed = nChars;
    str.m_buffer[nChars] = '\0';
    goto success;

error:
    KIS_ASSERT(false);
    goto cleanup;

success:
cleanup:
    va_end(args);

    return str;
}
