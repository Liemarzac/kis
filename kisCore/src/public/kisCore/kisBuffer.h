#pragma once

#include "kisCore.h"

#include "kisMem.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
class kisBuffer
{
public:
    kisBuffer(size_t size, uint32_t alignment = 16);
    ~kisBuffer();

    // Get the position of the current pointer in the buffer.
    uint8_t* ptr() const;

    // Offset the current pointer of size bytes.
    void commit(size_t size);

    // Copy data to the buffer and offset the current pointer of the number of bytes copied.
    void copyAndCommit(void* src, size_t size);

private:
    uint8_t* m_data;
    uint8_t* m_ptr;
    size_t m_size;
};

