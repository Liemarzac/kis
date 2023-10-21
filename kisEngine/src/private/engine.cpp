#include "engine.h"

#include <kisGraphics/graphics.h>

void kisEngine_Init(const void* metalLayer)
{
    kisGraphics_Init(metalLayer);
}

void kisEngine_Shutdown()
{
    kisGraphics_Shutdown();
}
