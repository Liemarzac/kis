#include <nng/nng.h>
#include <nng/protocol/pair1/pair.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static nng_socket s_socket;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static const char* s_listenURL = "tcp://0.0.0.0:8126";


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilderStartServer(const char* url)
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
void kisBuilderStopSever()
{
    // Nothing to do.
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
int main(int argc, const char * argv[])
{
    if(!kisBuilderStartServer(s_listenURL))
    {
        return 1;
    }

    kisBuilderStopSever();

    return 0;
}
