#include "kisCore.h"
#include "kisFile.h"
#include "kisMem.h"
#include "kisStringANSIStatic.h"

#include <filesystem>
#include <fstream>
#include <iostream>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
char g_kisFileDataPath[k_kisCoreMaxPathSize];


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
const char* kisFileDataPath()
{
    return g_kisFileDataPath;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFileOutputFilesystem()
{
    kisLogTableHeader("Filesystem");
    for(auto const& dirEntry : std::filesystem::recursive_directory_iterator(g_kisFileDataPath))
    {
        kisLogTableEntry(dirEntry.path().c_str());
    }
    kisLogTableFooter();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisFileBuffer kisFileBufferCreate(const char* path)
{
    kisStringANSIStatic<k_kisCoreMaxPathSize> fullyQualifiedPath;
    fullyQualifiedPath.concat("%s/%s", g_kisFileDataPath, path);

    kisFileBuffer fileBuffer;

    std::fstream file;
    file.open(fullyQualifiedPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if(file.is_open())
    {
        fileBuffer.m_size = file.tellg();
        file.seekg(0);
        fileBuffer.m_data = KIS_ALLOC(fileBuffer.m_size, kisMemTag::Core, "kisFileBuffer");
        file.read((char*)fileBuffer.m_data, fileBuffer.m_size);
        file.close();
    }
    else
    {
        fileBuffer.m_data = nullptr;
        fileBuffer.m_size = 0;
    }

    return fileBuffer;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFileBufferDestroy(kisFileBuffer& fileBuffer)
{
    if(fileBuffer.m_data)
    {
        kisMemFree(fileBuffer.m_data);
    }

    fileBuffer.m_data = nullptr;
    fileBuffer.m_size = 0;
}
