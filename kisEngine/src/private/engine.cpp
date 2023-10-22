#include "engine.h"

#include <kisGraphics/kisGraphics.h>

void kisEngine_Init(const void* metalLayer)
{
    kisGraphicsInit(metalLayer);
}

void kisEngine_Shutdown()
{
    kisGaphicsShutdown();
}
