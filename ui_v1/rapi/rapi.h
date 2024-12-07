#ifndef RAPI_H
#define RAPI_H

#include <stdio.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "interface/abstract_interface_provider.h"
#include "tools/rapi_names.hpp"
#include "app/device_manager.h"
#include "tools/helper.h"
#include "rapi_version.h"

class RAPI
{
public:
    static RAPI& instance()
    {
        static RAPI s;
        return s;
    }

    auto get_interfaces_info() {return interface_collector->get_interfaces_info();}
    //    auto get_interface(uint32_t id, std::shared_ptr<AbstractInterface> &iface) {return interface_collector->get_interface(id, iface);}

    auto get_devices() {return device_manager->get_devices();}

    template<class T>
    auto get_interface(T t) {return interface_collector->get_interface(t);}

//    std::shared_ptr<BasicProtocol> protocol;

    std::shared_ptr<InterfaceCollector> interface_collector;

    const int version[3] = {VERSION_RAPI_MAJOR, VERSION_RAPI_MINOR, VERSION_RAPI_PATCH};

private:
    RAPI()
    {
        PRINT_CTOR();
        PRINT_MSG("Version: " << std::to_string(version[0]) + "." + std::to_string(version[1]) + "." + std::to_string(version[2]));
        PRINT_MSG("CPP version: " << std::to_string(__cplusplus));

        interface_collector = std::shared_ptr<InterfaceCollector>(
                    new InterfaceCollector(
                        std::bind(&RAPI::set_parser_cb, this, std::placeholders::_1),
                        std::bind(&RAPI::remove_interface_signal, this, std::placeholders::_1)));

        setup_interfaces();

        //    interface_providers.emplace_back(std::unique_ptr<AbstractInterfaceProvider>(new TestInterfaceProvider(interface_collector)));

        device_manager = std::shared_ptr<DeviceManager>(new DeviceManager());

        //    protocol = std::shared_ptr<BasicProtocol>(new BasicProtocol(interface_collector));

    }
    ~RAPI()
    {
        PRINT_DTOR();
    }

    void setup_interfaces();


    RAPI(RAPI const&) = delete;
    RAPI& operator= (RAPI const&) = delete;


    std::vector<std::unique_ptr<AbstractInterfaceProvider> > interface_providers;

    std::shared_ptr<DeviceManager> device_manager;

    AbstractParser *default_parser = nullptr;
    void set_parser_cb(AbstractInterface *iface);
    void remove_interface_signal(int id);
};


#endif // RAPI_H
