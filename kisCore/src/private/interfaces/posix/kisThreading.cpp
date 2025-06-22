#include "kisThreading.h"

#include <pthread.h>


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static_assert(sizeof(kisThread::pimpl) == sizeof(pthread_t), "Incorrect pimpl size for kisThread");
static_assert(__alignof__(kisThread::pimpl) == kisThread::sAlignment, "Incorrect pimpl alignment for kisThread");

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static pthread_t* kis_PThreadPtr(kisThread& interface)
{
    return reinterpret_cast<pthread_t*>(interface.pimpl);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static pthread_t& kis_PThreadRef(kisThread& interface)
{
    pthread_t* pimpl = reinterpret_cast<pthread_t*>(interface.pimpl);
    return *pimpl;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisThread kisThread_Create(kisThreadFunc func, void* ctx)
{
    kisThread thread;
    KIS_CHECK(pthread_create(kis_PThreadPtr(thread), NULL, func, ctx) == 0);
    return thread;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisThread_Join(kisThread thread)
{
    void* retVal;
    KIS_CHECK(pthread_join(kis_PThreadRef(thread), &retVal));
}
