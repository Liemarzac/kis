#pragma once

#include "core.h"

#include <cstdio>
#include <cstring>
#include <stdarg.h>

template<size_t Size>
class kisStringANSIStatic
{
public:
    kisStringANSIStatic()
    {
        m_buffer[0] = '\0';
    }

    kisStringANSIStatic(const char* str)
    {
        m_buffer[0] = '\0';
        concat(str);
    }

    void concat(const char* format, ...)
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
    
    const char* c_str() const
    {
        return m_buffer;
    }

    uint32_t len() const
    {
        return m_nUsed;
    }

private:
    char m_buffer[Size];
    uint32_t m_nUsed = 0;
};
