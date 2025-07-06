#include "kisNNG.h"

#include "kisBuilderClientServer.h"


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisHash128 k_kisBuilderMsgSignature = {
    .bytes = {0x04, 0xfd, 0x5c, 0xae, 0x37, 0x9b, 0x81, 0xf7, 0x1d, 0x46, 0xce, 0xfc, 0xa8, 0x1e, 0x33, 0x63}
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<kisBuilderMsgType>
bool kisBuilder_ProcessMsg(const void* msgBuffer, size_t msgSize, nng_pipe pipe);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_BUILDER_NET_MSG(MsgName)        static kisBuilder_Process##MsgName##Msg s_kisBuilder##MsgName##MsgCallback = nullptr;                                                       \
                                            void kisBuilder_Set##MsgName##MsgCallback(kisBuilder_Process##MsgName##Msg callback){ s_kisBuilder##MsgName##MsgCallback = callback; }      \
                                            template<>                                                                                                                                  \
                                            bool kisBuilder_ProcessMsg<kisBuilderMsgType::MsgName>(const void* msgBuffer, size_t msgSize, nng_pipe nngPipe)                             \
                                            {                                                                                                                                           \
                                                if (msgSize != sizeof(kisBuilder##MsgName##Msg))                                                                                        \
                                                {                                                                                                                                       \
                                                    return false;                                                                                                                       \
                                                }                                                                                                                                       \
                                                if (s_kisBuilder##MsgName##MsgCallback == nullptr)                                                                                      \
                                                {                                                                                                                                       \
                                                    return false;                                                                                                                       \
                                                }                                                                                                                                       \
                                                return s_kisBuilder##MsgName##MsgCallback(*((kisBuilder##MsgName##Msg*)msgBuffer), nngPipe);                                            \
                                            }
#include "kisBuilderNetMsg.h"
#undef KIS_BUILDER_NET_MSG

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ProcessMsg(nng_msg* nngMsg)
{
    const void* msgBuffer = nng_msg_body(nngMsg);
    const size_t msgSize = nng_msg_len(nngMsg);

    if (msgSize < sizeof(kisBuilderBaseMsg))
    {
        return false;
    }

    const kisBuilderBaseMsg* baseMsg = reinterpret_cast<const kisBuilderBaseMsg*>(msgBuffer);

    if (baseMsg->m_signature != k_kisBuilderMsgSignature)
    {
        return false;
    }

    nng_pipe nngPipe = nng_msg_get_pipe(nngMsg);

    #define KIS_BUILDER_NET_MSG(MsgName) case kisBuilderMsgType::MsgName:{ return kisBuilder_ProcessMsg<kisBuilderMsgType::MsgName>(msgBuffer, msgSize, nngPipe); }
    switch (baseMsg->m_type)
    {
        #include "kisBuilderNetMsg.h"
        case kisBuilderMsgType::Count :
            return false;
    }
    #undef RCASSETMANAGER_NET_MSG

    return false;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ReceiveMsg(nng_socket nngSocket)
{
    nng_msg* nngMsg = nullptr;
    int recvRet = nng_recvmsg(nngSocket, &nngMsg, 0);

    bool ret = false;
    switch (recvRet)
    {
    case 0:
        kisLog("Message received");
        kisBuilder_ProcessMsg(nngMsg);
        ret = true;
        break;

    case NNG_EAGAIN:
    case NNG_ETIMEDOUT:
        ret = true;
        break;
    }

    if (nngMsg != nullptr)
    {
        nng_msg_free(nngMsg);
        nngMsg = nullptr;
    }

    return ret;
}

