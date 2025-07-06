#include "kisFile.h"

#include "kisBuilderClient.h"
#include "kisBuilderClientServer.h"
#include "kisMem.h"

#include <filesystem>
#include <fstream>
#include <iostream>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static kisFilePathString s_dataFilePath;


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
const char* kisFileDataPath()
{
    return s_dataFilePath.c_str();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFile_Init(const char* dataPath)
{
    s_dataFilePath = dataPath;
    kisString32 url = kisString32::format("tcp://192.168.1.160:%u", kisBuilder_GetServerDefaultListeningPort());
    kisBuilder_ClientConnect(url.c_str());
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFile_Shutdown()
{
    kisBuilder_ClientDisconnect();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisFileOutputFilesystem()
{
    kisLogTableHeader("Filesystem");
    for(auto const& dirEntry : std::filesystem::recursive_directory_iterator(s_dataFilePath.c_str()))
    {
        kisLogTableEntry(dirEntry.path().c_str());
    }
    kisLogTableFooter();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisFileBuffer kisFileBufferCreate(const char* path)
{
    kisFilePathString fullyQualifiedPath = kisFilePathString::format("%s" KIS_PATH_SEPARATOR "%s", s_dataFilePath.c_str(), path);

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
        kisBuilder_ClientRequestResource(path);
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
