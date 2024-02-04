#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "../tools/logger.h"
#include "../tools/debug_tools.h"
#include "../tools/rapi_names.hpp"
#include "../tools/concurrent_notifier.h"
#include "../tools/event_counter.h"

class Device_t
{
public:
    Device_t(int iface_id,
             uint8_t can_id,
             uint8_t can_state):
        iface_id(iface_id),
        can_id(can_id),
        can_state(can_state),
        timestamp(std::chrono::system_clock::now()),
        diff(LONG_MAX, LONG_MIN)
    {}

    int iface_id;
    uint8_t can_id;
    uint8_t can_state;

    std::chrono::system_clock::time_point timestamp;

    std::pair<long, long> diff;
    long diff_last_us;

    void update_hb()
    {
        auto now = std::chrono::system_clock::now();
        diff_last_us = static_cast<long>(
                    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timestamp).count());
        timestamp = now;
        if(diff_last_us < diff.first) {diff.first = diff_last_us;}
        if(diff_last_us > diff.second) {diff.second = diff_last_us;}

//        PRINT_MSG("[U]\tHB");
    }

    auto get_time_diff_us_last() const { return diff_last_us; }

    auto get_time_diff_us()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timestamp).count();
    }
};

class DeviceManager : public BasicInformator<RapiTypes::MsgPipeInfo>, public EventCounter
{
private:
    struct ListenerProductor_t
    {
        int pipe;
        int listener_id;
    };

public:
    DeviceManager():
        BasicInformator("Device Manager")
    {}

    std::list<ListenerProductor_t> listener_productors;

    void rx(const RapiTypes::MsgPipeInfo *info)
    {
        if(info->type == typeid(RapiTypes::MsgHb))
        {
            if(info->is_create)
            {
                PRINT_INFO("HB pipe detected: " << info->pipe_id);
                int listener_id;

                ContentProvider::get_instance().attach_listener(
                            info->pipe_id,
                            [&](std::type_index type, const void* data)
                {
                    if(type == typeid(RapiTypes::MsgHb))
                    {
                        receive(static_cast<const RapiTypes::MsgHb *>(data));
                    }
                    else
                    {
                        PRINT_ERROR("Device manager HB received the wrong format: " << type.name());
                    }
                },
                listener_id, "Global HB listener");
                listener_productors.push_back({info->pipe_id, listener_id});
            }
            else
            {
                PRINT_INFO("HB pipe removed;" << info->pipe_id);
                auto it = std::find_if(listener_productors.begin(),
                                       listener_productors.end(),
                                       [&](ListenerProductor_t &ref){return info->pipe_id == ref.pipe;});
                if(it != listener_productors.end())
                {
                    ContentProvider::get_instance().remove_listener(it->pipe, it->listener_id);
                }
            }
        }
    }

    void receive(const RapiTypes::MsgHb *info)
    {
        auto getStrOperStateName = [](uint8_t state) -> std::string
        {
            enum
            {
                CO_NMT_INITIALIZING = 0,      /**< Device is initializing */
                CO_NMT_PRE_OPERATIONAL = 127, /**< Device is in pre-operational state */
                CO_NMT_OPERATIONAL = 5,       /**< Device is in operational state */
                CO_NMT_STOPPED = 4,           /**< Device is stopped */
                CO_NMT_COLLISION = 3,         /**< Device is in collision state */
                CO_NMT_BOOT = 2               /**< Device is in bootloader state */
            };

            switch(state)
            {
                case CO_NMT_INITIALIZING: return "INIT";
                case CO_NMT_PRE_OPERATIONAL: return "PRE_OPER.";
                case CO_NMT_OPERATIONAL: return "OPER.";
                case CO_NMT_STOPPED: return "STOPPED";
                case CO_NMT_COLLISION: return "COLLISION";
                case CO_NMT_BOOT: return "BOOT";
                default: return ":" + std::to_string(state);
            }
        };

        //        static int cnt = 0;
        //        PRINT_MSG("HB received " <<
        //                  static_cast<int>(info->iface_id) << " " <<
        //                  static_cast<int>(info->can_id) << " " <<
        //                  static_cast<int>(info->state) << " >> " << cnt++);

        auto it = devices.begin();
        while(it != devices.end())
        {
            if(it->iface_id == info->iface_id &&
                    it->can_id == info->can_id)
            {
                if(it->can_state != info->state)
                {
                    PRINT_MSG("[STATE]\t" <<
                              static_cast<int>(info->iface_id) << " " <<
                              static_cast<int>(info->can_id) << " > " <<
                              static_cast<int>(info->state) << " (" << getStrOperStateName(info->state) << ")");
                    it->can_state = info->state;

                    increment_event_counter();
                }
                it->update_hb();
                return;
            }
            it++;
        }

        PRINT_MSG("[+D]\t" <<
                  static_cast<int>(info->iface_id) << " " <<
                  static_cast<int>(info->can_id) << " > " <<
                  static_cast<int>(info->state) << " (" << getStrOperStateName(info->state) << ")");
        devices.emplace_back(info->iface_id, info->can_id, info->state);

        increment_event_counter();
    }

    auto get_devices() {return devices;}

private:
    std::list<Device_t> devices;
};

#endif // DEVICEMANAGER_H
