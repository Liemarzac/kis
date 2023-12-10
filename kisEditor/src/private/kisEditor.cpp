#include "kisEditor.h"

#include <kisCore/kisCore.h>
#include <kisCore/kisStringANSIStatic.h>
#include <kisCore/kisArray.h>
#include <kisCore/kisBuffer.h>
#include <kisCore/kisFile.h>
#include <kisGraphics/kisGraphics.h>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#include <float.h>


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorLoadScene(const char* path)
{
    const aiScene* scene = aiImportFile(path,
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

        kisGraphicsCreateMesh(vertexBuffer.dataPointer(), vertexBuffer.num(), indexBufer.getStart(), mesh->mNumFaces * 3, sizeofIndex == sizeof(uint16_t) ? kisIndexBufferType::U16 : kisIndexBufferType::U32);
    }

    aiReleaseImport(scene);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorInit(const kisEditorInitParams* params)
{
    kisCoreInitParams coreInitParams;
    coreInitParams.m_dataPath = params->m_dataPath;
    kisCoreInit(coreInitParams);

    kisGraphicsInit(params->m_metalLayer);

    kisStringANSIStatic<k_kisCoreMaxPathSize> fullyQualifiedPath;
    fullyQualifiedPath.concat("%s/%s", kisFileDataPath(), "data/editor/plane.fbx");

    kisEditorLoadScene(fullyQualifiedPath.c_str());
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisEditorShutdown()
{
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

