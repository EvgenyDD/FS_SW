#ifndef INTERFACE_COLLECTOR_H
#define INTERFACE_COLLECTOR_H

#include <vector>

#include "abstract_interface.h"
#include "parsers/can/abstract_serializer.h"

#include "../tools/logger.h"
#include "../tools/concurrent_notifier.h"
#include "../tools/event_counter.h"

class InterfaceCollector : public EventCounter
{
public:
    InterfaceCollector(std::function<void(AbstractInterface*)> set_parser_cb,
                       std::function<void(int)> remove_interface_signal) :
        set_parser_cb(set_parser_cb),
        remove_interface_signal(remove_interface_signal)
    {
        PRINT_CTOR();
        pipe_iface = ContentProvider::get_instance().create_pipe<RapiTypes::MsgIfaceInfo>(ContentProvider::common_interface);
        thrInterfaceSupervisorDelete.start(std::bind(&InterfaceCollector::threadSupervisorInterfaceDelete, this));
    }

    ~InterfaceCollector()
    {
        PRINT_DTOR_PREPARE();

        thrInterfaceSupervisorDelete.terminate();

        /* Remove Interfaces */
        {
            std::lock_guard<std::mutex> lock(list_mutex);
            std::list<std::shared_ptr<AbstractInterface> >::iterator it = interfaces.begin();
            while(it != interfaces.end())
            {
                it = interfaces.erase(it);
            }
        }

        PRINT_DTOR();
    }    

    int threadSupervisorInterfaceDelete()
    {
        {
            std::lock_guard<std::mutex> lock(list_mutex);
            //                std::cout << "Size:" << interfaces.size() << std::endl;

            std::list<std::shared_ptr<AbstractInterface> >::iterator it = interfaces.begin();
            while(it != interfaces.end())
            {
                if((*it)->isTimeout())
                {
                    RapiTypes::MsgIfaceInfo info;
                    info.ptr = *it;
                    info.iface_id = (*it)->get_id();
                    info.is_create = false;
                    ContentProvider::get_instance().append_data<RapiTypes::MsgIfaceInfo>(pipe_iface, info);
                    PRINT_MSG("[-I]: {" << (*it)->get_connect_id() <<
                              "} " << (*it)->get_name() << " HB: " << (*it)->getHb());
                    remove_interface_signal(info.iface_id);
                    it = interfaces.erase(it);
                    increment_event_counter();
                }
                else if((*it)->interface_is_valid == false)
                {
                    if((*it)->interface_fault_lock == false)
                    {
                        std::cout << "[FAULT]\tInterface: " << (*it)->get_connect_id() <<
                            " Name: " << (*it)->get_name() << " Msg: " << (*it)->get_fault_message() << std::endl;
                        (*it)->interface_fault_lock = true;
                        increment_event_counter();
                    }
                    it++;
                }
                else
                {
                    it++;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 0;
    }

    template<class Interface, class Parser>
    void append_interface(uint32_t timeoutMs, std::string &&connectId, std::string &&name, AbstractSerializer *p_ser)
    {
        (void)p_ser;
        std::lock_guard<std::mutex> lock(list_mutex);

        auto iterator = std::find_if( std::begin(interfaces),
                                     std::end(interfaces),
                                     [&](const std::shared_ptr<AbstractInterface> &i){return std::forward<std::string>(connectId) == i->get_connect_id();});

        if(interfaces.end() == iterator)
        {
            /* append new interface */
            bool success_init = false;
            interfaces.emplace_back(
                std::shared_ptr<Interface>(new Interface(
                    success_init,
                    generate_id_listener(),
                    timeoutMs,
                    std::forward<std::string>(connectId),
                    std::forward<std::string>(name))));

            std::list<std::shared_ptr<AbstractInterface>>::iterator ref_interface = interfaces.end();
            ref_interface--;

            if(success_init)
            {
//                ref_interface->get()->set_parser(new Parser(
//                                                     ref_interface->get()->get_id(),
//                                                     ref_interface->get()->get_if_impl_mutex(),
//                                                     p_ser));
                set_parser_cb(ref_interface->get());
            }
            else
            {
                PRINT_ERROR("Interface failed to initialize");
            }

            PRINT_MSG("[+I]: {" << interfaces.back()->get_connect_id() <<
                      "} " << interfaces.back()->get_name());
            //            interfaces.back()->open();

            RapiTypes::MsgIfaceInfo iface_info{true, interfaces.back()->get_id(), interfaces.back()};
            ContentProvider::get_instance().append_data<RapiTypes::MsgIfaceInfo>(pipe_iface, iface_info);

            increment_event_counter();
        }
        else
        {
            /* update interface heartbeat */
            (*iterator)->update_heartbeat();
        }
    }

    //    bool get_iface_id(int iface_id, std::string &iface_id_string)
    //    {
    //        std::shared_ptr<AbstractInterface> iface;
    //        if(get_interface(iface_id, iface) == false)
    //        {
    //            iface_id_string = iface->get_name();
    //            return false;
    //        }
    //        else
    //        {
    //            PRINT_ERROR("Iface ID was not found with ID: " << iface_id);
    //            return true;
    //        }
    //    }

    //    bool get_iface_name(int iface_id, std::string name_iface_string)
    //    {
    //        std::shared_ptr<AbstractInterface> iface;
    //        if(get_interface(iface_id, iface) == false)
    //        {
    //            name_iface_string = iface->get_name();
    //            return false;
    //        }
    //        else
    //        {
    //            PRINT_ERROR("Iface ID was not found with ID: " << iface_id);
    //            return true;
    //        }
    //    }

    class InterfaceInfo
    {
    public:
        int id;
        std::string iface_id;
        std::string name;
        bool is_fault;
        bool is_fault_lock;
        std::string fault;

        InterfaceInfo(int id,
                      std::string connect_id,
                      std::string name,
                      bool is_fault,
                      bool is_fault_lock,
                      std::string fault) :
                                           id(id),
                                           iface_id(connect_id),
                                           name(name),
                                           is_fault(is_fault),
                                           is_fault_lock(is_fault_lock),
                                           fault(fault)
        {}
    };

    auto get_interfaces_info()
    {
        std::lock_guard<std::mutex> lock(list_mutex);
        std::vector<InterfaceInfo> list;

        std::list<std::shared_ptr<AbstractInterface> >::iterator it = interfaces.begin();
        while(it != interfaces.end())
        {
            list.emplace_back(
                (*it)->get_id(),
                (*it)->get_connect_id(),
                (*it)->get_name(),
                (*it)->interface_is_valid,
                (*it)->interface_fault_lock,
                (*it)->get_fault_message());
            it++;
        }
        return list;
    }

    std::shared_ptr<AbstractInterface> get_interface(int id)
    {
        std::lock_guard<std::mutex> lock(list_mutex);

        auto iterator = std::find_if( std::begin(interfaces),
                                     std::end(interfaces),
                                     [&](const std::shared_ptr<AbstractInterface> &i){return id == i->get_id();});

        if(interfaces.end() == iterator)
        {
            return nullptr;
        }
        else
        {
            return *iterator;
        }
    }

    bool compare_full(const std::string &one, const std::string &two)
    {
//        PRINT_MSG(one << " and " << two);
        return one == two;
    }
    bool compare_part(const std::string &one, const std::string &two)
    {
//        PRINT_MSG(one << " and " << two);
        for(size_t i=0; i<one.size() && i<two.size(); i++)
        {
            if(one[i] == '*' || two[i] == '*') return true;
            if(one[i] != two[i]) return false;
        }
        return true;
    }

    std::shared_ptr<AbstractInterface> get_interface(std::string id)
    {
        std::list<std::shared_ptr<AbstractInterface> >::iterator iterator;

        std::lock_guard<std::mutex> lock(list_mutex);

        if(StringHelper::contains(id, "*"))
        {
            iterator = std::find_if( std::begin(interfaces),
                                    std::end(interfaces),
                                    [&](const std::shared_ptr<AbstractInterface> &i){return compare_part(id, i->get_name());});
        }
        else
        {
            iterator = std::find_if( std::begin(interfaces),
                                    std::end(interfaces),
                                    [&](const std::shared_ptr<AbstractInterface> &i){return compare_full(id, i->get_name());});
        }

        if(interfaces.end() == iterator)
        {
            return nullptr;
        }
        else
        {
            return *iterator;
        }
    }
private:
    std::list<std::shared_ptr<AbstractInterface> > interfaces;

    static int generate_id_listener()
    {
        static int id_listener = 0;
        return id_listener++;
    }

    std::function<void(AbstractInterface*)> set_parser_cb;
    std::function<void(int)> remove_interface_signal;
    basic_thread thrInterfaceSupervisorDelete;

    std::mutex list_mutex;
    int pipe_iface = 0;
    };

#endif // INTERFACE_COLLECTOR_H
