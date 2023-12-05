#pragma once

#if __cplusplus
extern "C" {
#endif

typedef struct kisEditorInitParams
{
    const void* m_metalLayer;
    char m_dataPath[256];
} kisEditorInitParams;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorInit(const kisEditorInitParams* params);
void kisEditorShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorRender();
void kisEditorResize(unsigned int width, unsigned int height);

#if __cplusplus
}
#endif
