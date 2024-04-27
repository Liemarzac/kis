#include "kisEditor.h"

#include <kisCore/kisCore.h>
#include <kisCore/kisStringANSIStatic.h>
#include <kisCore/kisArray.h>
#include <kisCore/kisBuffer.h>
#include <kisCore/kisFile.h>
#include <kisCore/kisFixedArray.h>
#include <kisCore/kisTime.h>
#include <kisGraphics/kisGraphics.h>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include <float.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisEditor
{
    kisArray<kisMeshHandle> m_meshHandles;
    kisArray<kisMeshInstanceHandle> m_meshInstanceHandles;
    kisArray<kisTextureHandle> m_textureHandles;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisEditor* g_kisEditor = nullptr;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorLoadScene(const char* path)
{
    kisFilePathString fullyQualifiedPath = kisFileDataPath();
    fullyQualifiedPath.concat("/%s", path);

    const aiScene* scene = aiImportFile(fullyQualifiedPath.c_str(),
                             aiProcess_CalcTangentSpace      |
                             aiProcess_Triangulate           |
                             aiProcess_JoinIdenticalVertices |
                             aiProcess_SortByPType);

    KIS_ASSERT(scene != nullptr);

    for(size_t iMesh = 0; iMesh < scene->mNumMeshes; ++iMesh)
    {
        const aiMesh* mesh = scene->mMeshes[iMesh];

        kisArray<kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent> vertexBuffer((uint32_t)mesh->mNumVertices);

        const bool bHasUVs = mesh->HasTextureCoords(0);
        const bool bHasColor = mesh->HasVertexColors(0);
        const bool bHasTangentBitangent = mesh->HasTangentsAndBitangents();

        float AABB[2][3];
        AABB[0][0] = FLT_MAX;
        AABB[0][1] = FLT_MAX;
        AABB[0][2] = FLT_MAX;
        AABB[1][0] = -FLT_MAX;
        AABB[1][1] = -FLT_MAX;
        AABB[1][2] = -FLT_MAX;

        for(size_t iVertex = 0; iVertex < mesh->mNumVertices; iVertex++)
        {
            const aiVector3D* pos = &(mesh->mVertices[iVertex]);
            const aiVector3D* normal = &(mesh->mNormals[iVertex]);

            if (pos->x < AABB[0][0])
                AABB[0][0] = pos->x;
            if (pos->y < AABB[0][1])
                AABB[0][1] = pos->y;
            if (pos->z < AABB[0][2])
                AABB[0][2] = pos->z;
            if (pos->x > AABB[1][0])
                AABB[1][0] = pos->x;
            if (pos->y > AABB[1][1])
                AABB[1][1] = pos->y;
            if (pos->z > AABB[1][2])
                AABB[1][2] = pos->z;

            const aiVector3D* uv_or_color = nullptr;
            if(bHasUVs || bHasColor)
            {
                uv_or_color = &(mesh->mTextureCoords[0][iVertex]);
            }

            kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent& vertex = vertexBuffer.add();
            memcpy(vertex.m_position, pos, 3 * sizeof(float));

            if(bHasUVs)
            {
                const aiVector3D* uv = &mesh->mTextureCoords[0][iVertex];
                vertex.m_uv[0] = uv->x;
                vertex.m_uv[1] = uv->y;
            }
            else
            {
                vertex.m_uv[0] = 0.0f;
                vertex.m_uv[1] = 0.0f;
            }

            if(bHasColor)
            {
                const aiColor4D* color = mesh->mColors[0];
                vertex.m_color[0] = color->r;
                vertex.m_color[1] = color->g;
                vertex.m_color[2] = color->b;
            }
            else
            {
                vertex.m_color[0] = 1.0f;
                vertex.m_color[1] = 1.0f;
                vertex.m_color[2] = 1.0f;
            }

            vertex.m_normal[0] = normal->x;
            vertex.m_normal[1] = -normal->y;
            vertex.m_normal[2] = normal->z;

            if(bHasTangentBitangent)
            {
                const aiVector3D* tangent = &mesh->mTangents[iVertex];
                const aiVector3D* bitangent = &mesh->mBitangents[iVertex];
                vertex.m_tangent[0] = tangent->x;
                vertex.m_tangent[1] = tangent->y;
                vertex.m_tangent[2] = tangent->z;
                vertex.m_bitangent[0] = bitangent->x;
                vertex.m_bitangent[1] = bitangent->y;
                vertex.m_bitangent[2] = bitangent->z;
            }
            else
            {
                vertex.m_tangent[0] = 0.0f;
                vertex.m_tangent[1] = 1.0f;
                vertex.m_tangent[2] = 0.0f;
                vertex.m_bitangent[0] = 0.0f;
                vertex.m_bitangent[1] = 1.0f;
                vertex.m_bitangent[2] = 0.0f;
            }
        }

        // Loop through all indices and check the biggest one to see if they have to be 16 or 32 bit.
        uint32_t maxIndex = 0;
        for(size_t iFace = 0; iFace < mesh->mNumFaces; ++iFace)
        {
            const aiFace& face = mesh->mFaces[iFace];
            for(size_t iIndex = 0; iIndex < 3; ++iIndex)
            {
                if(face.mIndices[iIndex] > maxIndex)
                    maxIndex = face.mIndices[iIndex];
            }
        }

        size_t sizeofIndex = maxIndex >= USHRT_MAX ? sizeof(uint32_t) : sizeof(uint16_t);

        kisBuffer indexBufer(mesh->mNumFaces * 3 * sizeofIndex);

        if(sizeofIndex == sizeof(uint16_t))
        {
            for(size_t iFace = 0; iFace < mesh->mNumFaces; ++iFace)
            {
                const aiFace& face = mesh->mFaces[iFace];
                for(size_t iIndex = 0; iIndex < 3; ++iIndex)
                {
                    indexBufer.copyAndCommit(face.mIndices + iIndex, sizeof(uint16_t));
                }
            }
        }
        else
        {
            for(size_t iFace = 0; iFace < mesh->mNumFaces; ++iFace)
            {
                const aiFace& face = mesh->mFaces[iFace];
                for(size_t iIndex = 0; iIndex < 3; ++iIndex)
                {
                    indexBufer.copyAndCommit(face.mIndices + iIndex, sizeof(uint32_t));
                }
            }
        }

        // Populate material content..
        for(size_t iMaterial = 0; iMaterial < scene->mNumMaterials; iMaterial++)
        {
            const aiMaterial* material = scene->mMaterials[iMaterial];
            aiString aipath;

            if(material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                if(material->GetTexture(aiTextureType_DIFFUSE, 0, &aipath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
                {
                    kisFilePathString texturePath = path;
                    int iLastSlash = texturePath.lastOccurenceOf("/");
                    texturePath.substring(0, iLastSlash + 1);
                    texturePath.concat(aipath.C_Str());

                    kisFileBuffer textureFileBuffer = kisFileBufferCreate(texturePath.c_str());
                    kisDDSFileInfo ddsFileInfo;
                    if(kisGraphicsGetDDSFileInfo(textureFileBuffer.m_data, textureFileBuffer.m_size, ddsFileInfo) == true)
                    {
                        KIS_LOG_ASSERT(ddsFileInfo.m_width >= 4 && ddsFileInfo.m_height >= 4, "BC texture width and height must be at least 4 pixels");
                        const void* textureData = (uint8_t*)textureFileBuffer.m_data + ddsFileInfo.m_dataOffset;
                        const size_t textureSize = textureFileBuffer.m_size - ddsFileInfo.m_dataOffset;

                        uint32_t widthLog2 = 0;
                        uint32_t heightLog2 = 0;
                        kisLog2(ddsFileInfo.m_width, widthLog2);
                        kisLog2(ddsFileInfo.m_height, heightLog2);

                        // Discard the 2 mip levels below 4 pixels wide.
                        widthLog2 -= 2;
                        heightLog2 -= 2;

                        uint32_t nMipsToUse = kisMin(widthLog2, heightLog2);
                        nMipsToUse = kisMin(nMipsToUse, ddsFileInfo.m_mipMapCount);

                        kisTextureHandle textureHandle = kisGraphicsCreateTexture2D(textureData, textureSize, ddsFileInfo.m_width, ddsFileInfo.m_height, nMipsToUse, ddsFileInfo.m_format);
                        g_kisEditor->m_textureHandles.add(textureHandle);
                    }
                    else
                    {
                        KIS_LOG_ASSERT(false, "Could not read ddc file %s", texturePath.c_str());
                    }

                    kisFileBufferDestroy(textureFileBuffer);
                }
            }
        }

        kisMeshHandle meshHandle = kisGraphicsCreateMesh(vertexBuffer.dataPointer(), vertexBuffer.num(), indexBufer.getStart(), mesh->mNumFaces * 3, sizeofIndex == sizeof(uint16_t) ? kisIndexBufferType::U16 : kisIndexBufferType::U32);
        g_kisEditor->m_meshHandles.add(meshHandle);

        kisMeshInstanceHandle meshInstanceHandle = kisGraphicsAddMeshInstance(meshHandle);
        g_kisEditor->m_meshInstanceHandles.add(meshInstanceHandle);
    }

    aiReleaseImport(scene);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorUnloadScene()
{
    g_kisEditor->m_meshInstanceHandles.empty();

    for(kisMeshHandle meshHandle : g_kisEditor->m_meshHandles)
    {
        kisGraphicsDestroyMesh(meshHandle);
    }

    for(kisTextureHandle textureHandle : g_kisEditor->m_textureHandles)
    {
        kisGraphicsDestroyTexture(textureHandle);
    }

    g_kisEditor->m_meshHandles.empty();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorInit(const kisEditorInitParams* params)
{
    kisCoreInitParams coreInitParams;
    coreInitParams.m_dataPath = params->m_dataPath;
    kisCoreInit(coreInitParams);

    kisGraphicsInit(params->m_metalLayer);

    KIS_MEM_SCOPE_BEGIN(kisMemTag::Editor);

    g_kisEditor = KIS_NEW(kisMemTag::Editor, "editor") kisEditor();
    kisFilePathString path = "editor/plane.fbx";
    kisEditorLoadScene(path.c_str());
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorShutdown()
{
    kisEditorUnloadScene();

    KIS_DELETE(g_kisEditor);
    g_kisEditor = nullptr;

    KIS_MEM_SCOPE_END(kisMemTag::Editor);

    kisGraphicsShutdown();
    kisCoreShutdown();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorRender()
{
    static kisTime startTime = kisTimeNow();
    const double elapsed = kisTimeDelta(startTime, kisTimeNow());
    const float zRot = kisDegToRad((float)(90.0 * elapsed));

    kisMatrix4 orientation = glm::rotate(kisMatrix4(1.0f), zRot, kisVec3(0.0f, 0.0f, 1.0f));

    for(kisMeshInstanceHandle handle : g_kisEditor->m_meshInstanceHandles)
    {
        kisMeshInstance& instance = kisGraphicsGetMeshInstance(handle);
        instance.m_orientation = orientation;
    }

    kisGraphicsRender();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorResize(unsigned int width, unsigned int height)
{
    kisGraphicsResize(width, height);
}

