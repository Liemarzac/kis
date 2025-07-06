#include "engine.h"

#include <kisCore/kisFile.h>
#include <kisGraphics/kisGraphics.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineInit(const kisEngineInitParams* params)
{
    kisCoreInitParams coreInitParams;
    coreInitParams.m_dataPath = params->m_dataPath;
    kisCoreInit(coreInitParams);

    kisGraphicsInit(params->m_metalLayer);
    
    kisEngineLoad();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineShutdown()
{
    kisEngineUnload();
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

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineLoad()
{
    kisFileBufferCreate("root.bin");
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineUnload()
{
    
}

