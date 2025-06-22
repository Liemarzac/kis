#include "kisClientServer.h"
#include "kisServer.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
int main(int argc, const char * argv[])
{
    if(!kisBuilder_StartServer(s_listenURL))
    {
        return 1;
    }

    kisBuilder_StopServer();

    return 0;
}
