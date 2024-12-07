#ifndef IPROVIDER_H
#define IPROVIDER_H

#include <memory>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <algorithm>
#include <string>
#include <list>

#include "../tools/debug_tools.h"

#include "../parsers/abstract_parser.h"
#include "interface_collector.h"


class AbstractInterfaceProvider
{
public:
    AbstractInterfaceProvider(std::shared_ptr<InterfaceCollector> collector) :
        collector(collector)
    {
        PRINT_CTOR();
    }

    virtual ~AbstractInterfaceProvider()
    {
        PRINT_DTOR();
    }

protected:
    template<class Interface, class Parser, typename ...Args>
    void append_interface(Args&&... args)
    {
        collector->append_interface<Interface, Parser>(std::forward<Args>(args)...);
    }


private:
    std::shared_ptr<InterfaceCollector> collector;
};

#endif // IPROVIDER_H
