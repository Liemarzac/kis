#pragma once

#include "kisCore.h"
#include "kisHash.h"
#include "kisStringANSIStatic.h"

#include <nng/nng.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
extern kisHash128 k_kisBuilderMsgSignature;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_BUILDER_NET_MSG(MsgName) MsgName,
enum kisBuilderMsgType : uint8_t
{
    #include "kisBuilderNetMsg.h"
    Count
};
#undef KIS_BUILDER_NET_MSG

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisBuilderBaseMsg
{
    kisBuilderBaseMsg(kisBuilderMsgType type) :
        m_signature(k_kisBuilderMsgSignature)
        ,m_type(type)
    {
    }

    kisHash128 m_signature;
    kisBuilderMsgType m_type;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisBuilderResourceReqMsg : public kisBuilderBaseMsg
{
    kisBuilderResourceReqMsg() : kisBuilderBaseMsg(kisBuilderMsgType::ResourceReq) {}

    kisString64 m_path;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_BUILDER_NET_MSG(MsgName)        typedef bool(*kisBuilder_Process##MsgName##Msg)(const kisBuilder##MsgName##Msg& msg, nng_pipe pipe);                            \
                                            void kisBuilder_Set##MsgName##MsgCallback(kisBuilder_Process##MsgName##Msg callback);
#include "kisBuilderNetMsg.h"
#undef KIS_BUILDER_NET_MSG

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename Msg>
bool kisBuilder_SendMsg(const Msg& msg, nng_socket nngSocket, nng_pipe nngPipe)
{
    nng_msg* nngMsg = nullptr;

    nng_msg_alloc(&nngMsg, sizeof(msg));
    nng_msg_set_pipe(nngMsg, nngPipe);

    void* msgBody = nng_msg_body(nngMsg);
    memcpy(msgBody, &msg, sizeof(msg));

    if (nng_sendmsg(nngSocket, nngMsg, 0) != 0)
    {
        // If send fails, we are in charge to free the msg.
        nng_msg_free(nngMsg);
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ReceiveMsg(nng_socket nngSocket);
