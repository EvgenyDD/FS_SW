#ifndef RAPI_NAMES_HPP
#define RAPI_NAMES_HPP

#include <string>
#include <stdint.h>
#include <typeindex>
#include <memory>
#include <vector>
#include <string.h>

enum ErrorCodes
{
    Ok = 0,
    GenericError,
    InvalidType,
    Timeout,
    PipeNotFound,
    ListenerNotFound,
    IfaceNotFound,
    timeout,
    notImplemented,
    SystemError,
};

class AbstractInterface;


namespace RapiTypes
{

class ProtocolInfo
{
public:
    ProtocolInfo(int id) :
        iface_id(id)
    {}

    int iface_id;
};

class MsgHb : public ProtocolInfo
{
public:
    uint8_t can_id;
    uint8_t state;

    MsgHb(int id) :
        ProtocolInfo(id)
    {}

    MsgHb(int iface_id, uint8_t can_id, uint8_t state) :
        ProtocolInfo(iface_id), can_id(can_id), state(state)
    {}
};

class MsgCanFrame : public ProtocolInfo{
public:
    bool extended;
    bool remote;
    uint32_t header;
    std::vector<uint8_t> data;

    MsgCanFrame(int id) : ProtocolInfo(id) {}
};

class MsgEmcy : public ProtocolInfo
{
public:
    uint8_t can_id;
    uint16_t err_code;
    uint8_t err_register;
    uint8_t err_bit;
    uint8_t err_data[4];

    MsgEmcy(int id) : ProtocolInfo(id) {}

    MsgEmcy(int id,
                uint8_t can_id,
                uint16_t err_code,
                uint8_t err_register,
                uint8_t err_bit,
                const uint8_t *data) :
        ProtocolInfo(id),
        can_id(can_id),
        err_code(err_code),
        err_register(err_register),
        err_bit(err_bit)
    {
        memcpy(err_data, data, sizeof(err_data));
    }
};

class MsgNmt : public ProtocolInfo
{
public:
    MsgNmt(int id) : ProtocolInfo(id) {}
};

class MsgSync : public ProtocolInfo
{
public:
    MsgSync(int id) : ProtocolInfo(id) {}
};

class MsgTimestamp : public ProtocolInfo
{
public:
    MsgTimestamp(int id) : ProtocolInfo(id) {}
};

class MsgPdo : public ProtocolInfo
{
public:
    uint32_t cob_id;
    std::vector<uint8_t> data;

    MsgPdo(int id) : ProtocolInfo(id) {}
};

class MsgSdoR : public ProtocolInfo
{
public:
    uint8_t can_id;
    uint16_t index;
    uint8_t sub_index;
    uint32_t abort_code;
    std::vector<uint8_t> data;

    MsgSdoR(int id) : ProtocolInfo(id) {}
};

class MsgSdoT : public ProtocolInfo
{
public:
    uint8_t can_id;
    uint16_t index;
    uint8_t sub_index;
    uint32_t abort_code;

    MsgSdoT(int id) : ProtocolInfo(id) {}
};

class MsgPipeInfo
{
public:
    bool is_create;
    int pipe_id;
    std::type_index type = typeid(void);
};


class MsgIfaceInfo
{
public:
    bool is_create;
    int iface_id;
    std::shared_ptr<AbstractInterface> ptr;
};

}

#endif // RAPI_NAMES_HPP
