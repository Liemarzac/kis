#pragma once

#include "core.h"

#include <atomic>
#include <condition_variable>

class RingBuffer
{
public:
    RingBuffer();

private:
    uint8_t m_buffer[512];
};
