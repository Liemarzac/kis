#include "kisServer.h"
#include "kisThreading.h"

#include <kisCore/kisStringANSIStatic.h>


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
    if(!kisBuilder_StartServer())
    {
        return 1;
    }

    kisLog("kisBuilder server started");

    while(true)
    {
        kisBuilder_ServerReceive();
    }

    //kisBuilder_StopServer();

    return 0;
}
