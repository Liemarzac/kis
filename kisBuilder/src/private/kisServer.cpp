#include <nng/nng.h>
#include <nng/protocol/pair1/pair.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static nng_socket s_socket;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_StartServer(const char* url)
{
    // Restrict the number of threads created by nng.
    nng_init_set_parameter(NNG_INIT_MAX_TASK_THREADS, 2);
    nng_init_set_parameter(NNG_INIT_MAX_EXPIRE_THREADS, 1);
    nng_init_set_parameter(NNG_INIT_MAX_POLLER_THREADS, 1);
    nng_init_set_parameter(NNG_INIT_NUM_RESOLVER_THREADS, 1);

    if (nng_pair1_open_poly(&s_socket) != 0)
    {
        return false;
    }

    if (nng_listen(s_socket, url, NULL, 0) != 0)
    {
        return false;
    }

    nng_socket_set_ms(s_socket, NNG_OPT_RECVTIMEO, 100);
    nng_socket_set_ms(s_socket, NNG_OPT_SENDTIMEO, 100);

    return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisBuilder_StopServer()
{
    // Nothing to do.
}
