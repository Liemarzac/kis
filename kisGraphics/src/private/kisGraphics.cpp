#include "graphics.h"
#include "kisVulkan/kisVulkan.h"

void kisGraphicsInit(const void* metalLayer)
{
    kisVkInit(metalLayer);
}

void kisGaphicsShutdown()
{
    kisVkShutdown();
}
