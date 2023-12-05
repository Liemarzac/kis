#include "kisEditor.h"

#include <kisCore/kisCore.h>
#include <kisCore/kisStringANSIStatic.h>
#include <kisCore/kisFile.h>
#include <kisGraphics/kisGraphics.h>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

const aiScene* g_aiScene = nullptr;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorInit(const kisEditorInitParams* params)
{
    kisCoreInitParams coreInitParams;
    coreInitParams.m_dataPath = params->m_dataPath;
    kisCoreInit(coreInitParams);

    kisGraphicsInit(params->m_metalLayer);

    kisStringANSIStatic<k_kisCoreMaxPathSize> fullyQualifiedPath;
    fullyQualifiedPath.concat("%s/%s", kisFileDataPath(), "data/editor/male_character_bow.fbx");

    // Start the import on the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll t
    // probably to request more postprocessing than we do in this example.
    g_aiScene = aiImportFile(fullyQualifiedPath.c_str(),
      aiProcess_CalcTangentSpace       |
          aiProcess_Triangulate            |
          aiProcess_JoinIdenticalVertices  |
          aiProcess_SortByPType);

    KIS_ASSERT(g_aiScene != nullptr);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorShutdown()
{
    aiReleaseImport(g_aiScene);
    g_aiScene = nullptr;

    kisGraphicsShutdown();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorRender()
{
    kisGraphicsRender();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorResize(unsigned int width, unsigned int height)
{
    kisGraphicsResize(width, height);
}

