#include "kisTime.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisTime kisTimeNow()
{
    return std::chrono::high_resolution_clock::now();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
double kisTimeDelta(kisTime start, kisTime end)
{
    std::chrono::duration<double> diff = end - start;
    return diff.count();
}
