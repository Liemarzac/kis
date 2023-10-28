#pragma once

#if __cplusplus
extern "C" {
#endif

typedef struct kisEngineInitParams
{
    const void* m_metalLayer;
    char m_dataPath[256];
} kisEngineInitParams;

void kisEngineInit(const kisEngineInitParams* params);

void kisEngineShutdown();


#if __cplusplus
}
#endif
