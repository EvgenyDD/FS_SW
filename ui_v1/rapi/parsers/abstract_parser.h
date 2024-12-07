#ifndef I_CAN_INTERFACE_H
#define I_CAN_INTERFACE_H

#include <stdint.h>
#include <iostream>
#include <list>
#include <chrono>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <shared_mutex>
#include <thread>
#include <typeinfo>

#include "../tools/debug_tools.h"
#include "../tools/concurrent_notifier.h"
#include "../tools/helper.h"
#include "../tools/rapi_names.hpp"
#include "../tools/basic_thread.h"


class AbstractParser
{
public:
    AbstractParser(int iface_id, std::shared_timed_mutex &if_impl_mutex) :
        iface_id(iface_id),
        if_impl_mutex(if_impl_mutex)
    {}
    virtual ~AbstractParser(){}

    void set_tx(std::function<ErrorCodes(const std::vector<uint8_t>&)> tx_method) {this->tx_method = tx_method;}

    int get_iface_id() const { return iface_id; }

    virtual ErrorCodes rx_data(const std::vector<uint8_t> &bytes __attribute__((unused)))
    {return ErrorCodes::notImplemented;}

private:
    int iface_id;

protected:
    std::function<ErrorCodes(const std::vector<uint8_t>&)> tx_method =
            [](const std::vector<uint8_t>&data __attribute__((unused))){ return ErrorCodes::notImplemented; };

    std::shared_timed_mutex &if_impl_mutex;
};


#endif /* I_CAN_INTERFACE_H */
