#pragma once

#if __cplusplus
extern "C" {
#endif

typedef struct kisEngineInitParams
{
    const void* m_metalLayer;
    char m_dataPath[256];
} kisEngineInitParams;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineInit(const kisEngineInitParams* params);
void kisEngineShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineRender();
void kisEngineResize(unsigned int width, unsigned int height);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEngineLoad();
void kisEngineUnload();

#if __cplusplus
}
#endif
