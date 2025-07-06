#include "kisBuilderClient.h"
#include "kisBuilderClientServer.h"
#include "kisStringANSIStatic.h"
#include "kisNNG.h"

#include <nng/protocol/pair1/pair.h>


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static nng_socket   s_nngSocket;
static nng_dialer   s_nngDialer;
static bool         s_isConnected = false;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ClientConnect(const char* url)
{
    bool bIsSocketOpened = false;

    // Restrict the number of threads created by nng.
    nng_init_set_parameter(NNG_INIT_MAX_TASK_THREADS, 2);
    nng_init_set_parameter(NNG_INIT_MAX_EXPIRE_THREADS, 1);
    nng_init_set_parameter(NNG_INIT_MAX_POLLER_THREADS, 1);
    nng_init_set_parameter(NNG_INIT_NUM_RESOLVER_THREADS, 1);

    if(nng_pair_open(&s_nngSocket) != 0)
    {
        goto error;
    }
    bIsSocketOpened = true;

    if(nng_dial(s_nngSocket, url, &s_nngDialer, 0) != 0)
    {
        goto error;
    }

    goto success;

error:
    if(bIsSocketOpened)
    {
        nng_socket_close(s_nngSocket);
    }
    return false;

success:
    s_isConnected = true;
    return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisBuilder_ClientDisconnect()
{
    if(!s_isConnected)
    {
        return;
    }

    nng_socket_close(s_nngSocket);
    s_isConnected = false;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ClientRequestResource(const kisString64& path)
{
    kisBuilderResourceReqMsg req;
    req.m_path = path;

    nng_pipe pipe = {0};
    return kisBuilder_SendMsg(req, s_nngSocket, pipe);
}

