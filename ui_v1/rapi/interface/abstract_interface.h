#ifndef I_INTERFACE_H
#define I_INTERFACE_H

#include <stdint.h>
#include <iostream>
#include <list>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <stdint.h>

#include "../tools/debug_tools.h"
#include "../tools/basic_thread.h"
#include "../tools/rapi_names.hpp"

#include "../parsers/abstract_parser.h"

class Watchdog
{
protected:
    uint32_t timeout_ms;

    Watchdog(uint32_t timeoutMs) :
        timeout_ms(timeoutMs)
    {
        update_heartbeat();
    }
    ~Watchdog(){}

public:
    bool isTimeout()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_time_point).count() >= timeout_ms;
    }

    void update_heartbeat()
    {
        last_time_point = std::chrono::system_clock::now();
    }

    auto getHb() {return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_time_point).count();}

private:
    std::chrono::system_clock::time_point last_time_point;
};

class  AbstractInterface : public Watchdog
{
public:
    enum class State{
        Closed=0,
        OpenRequest,
        Opened,
        Error
    };

    AbstractInterface(int iface_id, uint32_t timeoutMs, std::string &&connectId, std::string &&name) :
        Watchdog(timeoutMs),
        iface_id(iface_id),
        connect_id(std::forward<std::string>(connectId)),
        name(std::forward<std::string>(name))
    {
    }

    AbstractInterface() :
        Watchdog(0)
    {}

    ~AbstractInterface(){}

    const std::string &get_connect_id() const {return connect_id;}
    const std::string &get_name() const {return name;}
    const std::string &get_fault_message() const {return interface_fault_message;}
    auto get_id() const {return iface_id;}

    auto get_ptr_impl() const {return p_impl;}
    auto &get_if_impl_mutex() {return if_impl_mutex;}

    void set_parser(AbstractParser *iface)
    {
        std::unique_lock<std::shared_timed_mutex> lock(if_impl_mutex);
        p_impl.reset(iface);
    }

    virtual ErrorCodes tx_data(const std::vector<uint8_t> &data __attribute__((unused)))
    {return ErrorCodes::notImplemented;}

    std::atomic_bool interface_is_valid{true};
    std::atomic_bool interface_is_locked{false};
    std::atomic_bool interface_fault_lock{false};

protected:
    basic_thread thread_receive;

    std::string interface_fault_message{""};

    std::shared_ptr<AbstractParser> p_impl;
    std::shared_timed_mutex if_impl_mutex;

private:      
    std::atomic<State> state{State::Closed};

    int iface_id;
    std::string connect_id;
    std::string name;

};

#endif // I_INTERFACE_H
