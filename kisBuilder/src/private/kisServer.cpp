#include "kisServer.h"

#include <kisCore/kisBuilderClientServer.h>
#include <kisCore/kisNNG.h>
#include <kisCore/kisStringANSIStatic.h>

#include <nng/protocol/pair1/pair.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static nng_socket s_nngSocket;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ServerProcessResourceReqMsg(const kisBuilderResourceReqMsg& msg, nng_pipe nngPipe)
{
    kisLog("Received resource request: %s", msg.m_path.c_str());
    return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_StartServer()
{
    kisBuilder_SetResourceReqMsgCallback(kisBuilder_ServerProcessResourceReqMsg);

    // Restrict the number of threads created by nng.
    nng_init_set_parameter(NNG_INIT_MAX_TASK_THREADS, 2);
    nng_init_set_parameter(NNG_INIT_MAX_EXPIRE_THREADS, 1);
    nng_init_set_parameter(NNG_INIT_MAX_POLLER_THREADS, 1);
    nng_init_set_parameter(NNG_INIT_NUM_RESOLVER_THREADS, 1);

    if (nng_pair1_open_poly(&s_nngSocket) != 0)
    {
        return false;
    }

    kisString32 listenURL = kisString32::format("tcp://0.0.0.0:%u", kisBuilder_GetServerDefaultListeningPort());
    if (nng_listen(s_nngSocket, listenURL.c_str(), NULL, 0) != 0)
    {
        return false;
    }

    nng_socket_set_ms(s_nngSocket, NNG_OPT_RECVTIMEO, 100);
    nng_socket_set_ms(s_nngSocket, NNG_OPT_SENDTIMEO, 100);

    return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisBuilder_StopServer()
{
    // Nothing to do.
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ServerReceive()
{
    return kisBuilder_ReceiveMsg(s_nngSocket);
}
