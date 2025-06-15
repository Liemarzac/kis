#pragma once

#include "kisNet.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisNetServerSocketPImpl
{
    int socket;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisNetClientSocketPImpl
{
    int socket;
    struct sockaddr_in serverAddr;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static_assert(sizeof(kisNetServerSocket::m_impl) == sizeof(kisNetServerSocketPImpl));

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static_assert(sizeof(kisNetClientSocket::m_impl) == sizeof(kisNetClientSocketPImpl));


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
int getServerPosixSocket(kisNetServerSocket& kisSocket)
{
    return *((int*)&kisSocket.m_impl);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void setServerPosixSocket(kisNetServerSocket& socket, int posixSocket)
{
    *((int*)&socket.m_impl) = posixSocket;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
int getClientPosixSocket(kisNetClientSocket& kisSocket)
{
    return *((int*)&kisSocket.m_impl);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void setClientPosixSocket(kisNetClientSocket& socket, int posixSocket)
{
    *((int*)&socket.m_impl) = posixSocket;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisNetInitServerSocket(kisNetServerSocket& kisSocket, int port)
{
    int posixSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(posixSocket == -1)
    {
        return false;
    }

    setServerPosixSocket(kisSocket, posixSocket);

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if(bind(posixSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))
    {
        return false;
    }
    
    return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisNetInitClientSocket(kisNetClientSocket& kisSocket, const char* serverIP, int port)
{
    int posixSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(posixSocket == -1)
    {
        return false;
    }

    setClientPosixSocket(kisSocket, posixSocket);
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    if(inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0)
    {
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisNetServerSocket::kisNetServerSocket()
{
    setServerPosixSocket(*this, -1);
}

