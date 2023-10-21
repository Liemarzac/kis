#include "graphics.h"
#include "vulkan/vulkan.h"

void kisGraphics_Init(const void* metalLayer)
{
    kisVkInit(metalLayer);
}

void kisGraphics_Shutdown()
{
    kisVkShutdown();
}
