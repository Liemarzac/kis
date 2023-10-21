#include "graphics.h"
#include "kisVulkan/kisVulkan.h"

void kisGraphics_Init(const void* metalLayer)
{
    kisVkInit(metalLayer);
}

void kisGraphics_Shutdown()
{
    kisVkShutdown();
}
