#pragma once

#include "kisCore.h"
#include "kisStringANSIStatic.h"

#include <stddef.h>

typedef kisStringANSIStatic<k_kisCoreMaxPathSize> kisFilePathString;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(_WIN32)
#define KIS_PATH_SEPARATOR "\\"
#else
#define KIS_PATH_SEPARATOR "/"
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisFileBuffer
{
    void* m_data;
    size_t m_size;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
const char* kisFileDataPath();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFileOutputFilesystem();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisFileBuffer kisFileBufferCreate(const char* path);
void kisFileBufferDestroy(kisFileBuffer& fileBuffer);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFile_Init(const char* dataPath);
void kisFile_Shutdown();
