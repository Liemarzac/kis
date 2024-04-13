#include "engine.h"

#include <kisCore/kisCore.h>
#include <kisGraphics/kisGraphics.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineInit(const kisEngineInitParams* params)
{
    kisCoreInitParams coreInitParams;
    coreInitParams.m_dataPath = params->m_dataPath;
    kisCoreInit(coreInitParams);

    kisGraphicsInit(params->m_metalLayer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineShutdown()
{
    kisGraphicsShutdown();
    kisCoreShutdown();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineRender()
{
    kisGraphicsRender();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineResize(unsigned int width, unsigned int height)
{
    kisGraphicsResize(width, height);
}

