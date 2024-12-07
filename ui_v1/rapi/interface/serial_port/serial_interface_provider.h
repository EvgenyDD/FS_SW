#ifndef SERIALPORTPROVIDER_H
#define SERIALPORTPROVIDER_H

#include <condition_variable>
#include <memory>

#include "../abstract_interface_provider.h"
#include "asuit_protocol.h"

#include "serial.hpp"
#include "serial_interface.h"

class SerialInterfaceProvider : public AbstractInterfaceProvider
{
public:
    SerialInterfaceProvider(std::shared_ptr<InterfaceCollector> collector) :
        AbstractInterfaceProvider(collector)
    {
        PRINT_CTOR();
        thread_discovery.start(std::bind(&SerialInterfaceProvider::threadDiscovery, this));
    }

    virtual ~SerialInterfaceProvider()
    {
        PRINT_DTOR_PREPARE();
        thread_discovery.terminate();
        PRINT_DTOR();
    }

private:
    //https://github.com/wjwwood/serial

    basic_thread thread_discovery;

    int threadDiscovery()
    {
        std::vector<serial::PortInfo> devices_found = serial::PortList::list_ports();

#ifdef WIN32
        const std::string compare = "USB\\VID_0483&PID_5740&REV_0200";
#else
        const std::string compare = "USB VID:PID=0483:5740 SNR=301";
#endif

        std::vector<serial::PortInfo>::iterator iter = devices_found.begin();

        while( iter != devices_found.end() )
        {
            serial::PortInfo device = *iter++;
            if(device.hardware_id.c_str() == compare)
            {
//                PRINT_MSG(device.port.c_str() << " " << /*device.description.c_str() << " " <<*/ device.hardware_id.c_str());

                std::string connId = device.port.c_str();

                append_interface<SerialInterface, ASuitProtocol>(
                            900,
                            std::string(connId),
                            std::string(connId),
                            nullptr);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 0;
    }
};

#endif // SERIALPORTPROVIDER_H
