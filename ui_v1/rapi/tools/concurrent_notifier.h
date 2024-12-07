#ifndef CONCURRENT_NOTIFIER_H
#define CONCURRENT_NOTIFIER_H

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <typeinfo>
#include <typeindex>
#include <vector>

#include "debug_tools.h"
#include "rapi_names.hpp"
#include "helper.h"


class ContentProvider
{
public:
    static const int common_interface = -1;

    struct listener_t
    {
        int id;
        std::function<void(std::type_index type, const void*)> notify;
        std::string name;
        listener_t(int id,
                   std::function<void(std::type_index type, const void*)> notify,
                   std::string name) :
            id(id),
            notify(notify),
            name(name)
        {}
    };

    struct smart_pipe_cont_t
    {
        int id;
        int iface_id;
        const std::type_info& type;
        std::list<listener_t> listeners;
    };

    int find_pipe(int iface_id, const std::type_info &type)
    {
        auto p = find_if(pipes.begin(), pipes.end(), [&] (const smart_pipe_cont_t& s) { return (s.iface_id == iface_id) && (s.type == type); } );
        if(p == pipes.end())
        {
            return -1;
        }
        else
        {
            return p->id;
        }
    }

private:
    auto find_pipe(int id)
    {
        return find_if(pipes.begin(), pipes.end(), [&] (const smart_pipe_cont_t& s) { return s.id == id; } );
    }



    auto find_listener(std::list<smart_pipe_cont_t>::iterator &pipe, int id)
    {
        return find_if(pipe->listeners.begin(),
                       pipe->listeners.end(),
                       [&] (const listener_t& s) { return s.id == id; } );
    }

public:
    static ContentProvider &get_instance()
    {
        static ContentProvider instance;
        return instance;
    }

    template<class T>
    int create_pipe(int iface_id)
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);

        auto new_id = generate_id_pipe();
        PRINT_MSG("[+P]: " << new_id << " " << typeid(T).name());
        pipes.push_back({
                            new_id,
                            iface_id,
                            typeid(T),
                            std::list<listener_t>()
                        });

        if(new_id != master_pipe) /* each new pipe must be notifiable */
        {
            RapiTypes::MsgPipeInfo pipe_info{true, new_id, typeid(T)};
            append_data<RapiTypes::MsgPipeInfo>(master_pipe, pipe_info);
        }

        return new_id;
    }

    int remove_pipe(int pipe)
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        if(pipe == master_pipe)
        {
            PRINT_ERROR("Master pipe can't be deleted by clients! Index: " << pipe);
            return ErrorCodes::PipeNotFound;
        }

        auto p = find_pipe((pipe));

        if(p == pipes.end())
        {
            PRINT_ERROR("Pipe was not found: " << pipe);
            return ErrorCodes::PipeNotFound;
        }

        PRINT_MSG("[-P]: " << pipe << " " << p->type.name());

        RapiTypes::MsgPipeInfo pipe_info{false, pipe, p->type};
        append_data<RapiTypes::MsgPipeInfo>(master_pipe, pipe_info);

        pipes.erase(p);
        return ErrorCodes::Ok;
    }

    template<class T>
    int append_data(int pipe, T &data)
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        auto p = find_pipe((pipe));

        if(p == pipes.end())
        {
            PRINT_ERROR("Pipe was not found: " << pipe);
            return ErrorCodes::PipeNotFound;
        }
        if(p->type != typeid(T))
        {
            PRINT_ERROR("Pipe format: " << p->type.name() << " but data is " << typeid(T).name());
            return ErrorCodes::InvalidType;
        }

        for(auto &n : p->listeners)
        {
            n.notify(p->type, &data);
        }
        return ErrorCodes::Ok;
    }

    int attach_listener(
            int pipe,
            std::function<void(std::type_index type, const void*)> notifyable,
            int &listener_id,
            std::string listener_name = "Undefined")
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        auto p = find_pipe((pipe));

        if(p == pipes.end())
        {
            PRINT_ERROR("Pipe was not found: " << pipe);
            return ErrorCodes::PipeNotFound;
        }

        listener_id = generate_id_listener();
        PRINT_MSG("[+L]: " << listener_id << " \"" << listener_name <<"\" attached to pipe: " << pipe << " of type " << p->type.name());

        p->listeners.emplace_back(listener_t(
                                      listener_id,
                                      notifyable,
                                      listener_name));

        return ErrorCodes::Ok;
    }

    int remove_listener(
            int pipe,
            int listener)
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        auto p = find_pipe((pipe));

        if(p == pipes.end())
        {
            PRINT_ERROR("Pipe was not found: " << pipe);
            return ErrorCodes::PipeNotFound;
        }

        auto l = find_listener(p, listener);

        if(l == p->listeners.end())
        {
            PRINT_ERROR("Listener was not found: " << pipe << " " << listener);
            return ErrorCodes::ListenerNotFound;
        }

        PRINT_MSG("[-L] " << listener << " \"" << l->name << " removed from pipe: " << pipe << " of type " << p->type.name());
        p->listeners.erase(l);

        return ErrorCodes::Ok;
    }

    int get_master_pipe_informator() const {return master_pipe;}

    void print()
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        PRINT_MSG("============ PIPES ===========");
        for(auto &p : pipes)
        {
            PRINT_MSG("| " << p.id << "\t" <<
                      "Lstn cnt: " << p.listeners.size() << "\t" <<
                      p.type.name());
        }
        PRINT_MSG("==============================");
    }

private:
    std::list<smart_pipe_cont_t> pipes;
    int master_pipe = 0;

    static int generate_id_pipe()
    {
        static int id_pipe = 0;
        return id_pipe++;
    }

    static int generate_id_listener()
    {
        static int id_listener = 0;
        return id_listener++;
    }

    ContentProvider() :
        master_pipe(create_pipe<RapiTypes::MsgPipeInfo>(ContentProvider::common_interface))
    {
        PRINT_CTOR();
        std::lock_guard<std::recursive_mutex> lock(mtx);
    }

    ~ContentProvider()
    {
        PRINT_DTOR();
        std::lock_guard<std::recursive_mutex> lock(mtx);
        pipes.clear();
    }

    std::recursive_mutex mtx;
};


template<class T>
class BasicInformator
{
private:
    int listener_id;

public:
    void receiver(std::type_index type, const void *data)
    {
        if(type == typeid(T))
        {
            rx(static_cast<const T*>(data));
        }
        else
        {
            PRINT_ERROR("Wrong! " << __FUNCTION__);
        }
    }

    virtual void rx(const T *data) = 0;

    virtual ~BasicInformator()
    {
        ContentProvider::get_instance().remove_listener(
                    ContentProvider::get_instance().get_master_pipe_informator(),
                    listener_id);
    }

    BasicInformator(std::string listener_name)
    {
        ContentProvider::get_instance().attach_listener(
                    ContentProvider::get_instance().get_master_pipe_informator(),
                    [&](std::type_index type, const void* data){receiver(type, data);},
        listener_id,
                listener_name);
    }
};


class AsyncReceiver
{
public:
    /**
     * @brief wait received event
     * @param timeout_ms
     * @return
     */
    int wait(uint32_t timeout_ms)
    {
        using namespace std::chrono_literals;
        std::unique_lock<std::mutex> lk(mtx);
        return cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&](){return triggered;}) == 0;
    }

    void reset()
    {
        triggered = false;
    }

    void signal()
    {
        std::unique_lock<std::mutex> lk(mtx);
        triggered = true;
        cv.notify_one();
    }

private:
    std::condition_variable cv;
    std::mutex mtx;
    bool triggered = false;
};

template<class T>
class AsyncMultiReceiver
{
public:
    /**
     * @brief wait received event
     * @param timeout_ms total timeout
     * @param m2m_timeout_msmax timeout between messages
     * @return
     */
    int wait_multi(uint32_t timeout_ms, uint32_t m2m_timeout_ms)
    {
        timeout_m2m = m2m_timeout_ms;
        std::unique_lock<std::mutex> lk(mtx);
        auto state = cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&]()
                                 { return (std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::system_clock::now() - timestamp_start).count() > timestamp_acc /*&& count_rx == 0*/); });
//        PRINT_MSG("SSS" << state << "." << count_rx << ":" << std::chrono::duration_cast<std::chrono::milliseconds>
//                    (std::chrono::system_clock::now() - timestamp_start).count() <<
//                    "#" << timestamp_acc);
        active = false;
        return state == 0;
    }

    int wait_single(uint32_t timeout_ms)
    {
        std::unique_lock<std::mutex> lk(mtx);
        auto state = cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&]()
                                 { return (std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::system_clock::now() - timestamp_start).count() > timeout_ms /*&& count_rx == 0*/); });
//        PRINT_MSG("SSS" << state << "." << count_rx << ":" << std::chrono::duration_cast<std::chrono::milliseconds>
//                    (std::chrono::system_clock::now() - timestamp_start).count() <<
//                    "#" << timestamp_acc);
        active = false;
        return state == 0;
    }

    void reset()
    {
        timestamp_start = std::chrono::system_clock::now();
        timestamp_acc = 0;
        count_rx = 0;
        v.clear();
        active = true;
    }

    void signal(T &&t)
    {
        if(!active) {return;}
        std::unique_lock<std::mutex> lk(mtx);
        v.push_back(std::forward<T>(t));
        uint32_t diff = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::system_clock::now() - timestamp_start).count();
        timestamp_acc = diff + timeout_m2m;
        count_rx++;
        cv.notify_one();
    }

    void signal(const T *data, size_t len)
    {
        if(!active) {return;}
        std::unique_lock<std::mutex> lk(mtx);
        std::vector<T> t;
        t.assign(data, data + len);
        VectorHelper::append(v, t);
        uint32_t diff = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::system_clock::now() - timestamp_start).count();
        timestamp_acc = diff + timeout_m2m;
        count_rx++;
        cv.notify_one();
    }

    auto get_rx() const {return v;}

private:
    std::condition_variable cv;
    std::mutex mtx;
    std::vector<T> v;

    uint32_t timeout_m2m = 0;

    std::atomic<bool> active{false};

    std::chrono::system_clock::time_point timestamp_start;
    std::atomic<uint32_t> timestamp_acc = 0, count_rx = 0;
};

#endif // CONCURRENT_NOTIFIER_H
