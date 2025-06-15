#pragma once

#include "kisCore.h"

#if defined(__APPLE__)
const size_t k_kisNetSocketImplSize = sizeof(int);
const size_t k_kisNetAlignofSocketImpl = alignof(int);
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisNetServerSocket
{
    kisNetServerSocket();

    alignas(k_kisNetAlignofSocketImpl)
    uint8_t m_impl[k_kisNetSocketImplSize];
    int port = 0;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisNetClientSocket
{
    kisNetClientSocket();

    alignas(k_kisNetAlignofSocketImpl)
    uint8_t m_impl[k_kisNetSocketImplSize];
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisNetInitServerSocket(kisNetServerSocket& kisSocket, int port);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisNetInitClientSocket(kisNetClientSocket& kisSocket, const char* serverAddress, int port);
