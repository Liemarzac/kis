#include "kisGraphics.h"
#include "kisVk/kisVk.h"

void kisGraphicsInit(const void* metalLayer)
{
    kisVkInit(metalLayer);
}

void kisGaphicsShutdown()
{
    kisVkShutdown();
}
