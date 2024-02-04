#ifndef SMART_THREAD_H
#define SMART_THREAD_H

#include <thread>
#include <atomic>
#include <functional>
#include "debug_tools.h"
#include <condition_variable>

class basic_thread
{
public:
    ~basic_thread()
    {
        terminate();
    }

    /**
     * @brief Note: fun returns negative numbers - to wait time in us.
     *
     * @param fun
     */
    void start(std::function<int(void)> fun)
    {
        terminate();

        this->fun = fun;
        flagActive = true;
        thread.reset(new std::thread(&basic_thread::process, this));
    }

    void terminate()
    {
        flagActive = false;

        cv_delay.notify_all(); // terminate delays

        if(thread)
        {
            if(thread->joinable())
            {
                thread->join();
            }
        }
    }


    void delay(uint32_t delay_us)
    {
        std::unique_lock<std::mutex> lock(mtx_delay);
        cv_delay.wait_for(lock, std::chrono::microseconds(delay_us));
    }

    bool isActive() const { return flagActive; }

private:
    void process()
    {
        while(flagActive)
        {
            int sts = fun();
            if(sts > 0)
            {
                flagActive = false;
                PRINT_INFO("[XX]\tThread has terminated itself");
            }
            else
            {
                delay(static_cast<uint32_t>(-sts));
            }
        }
    }

    std::condition_variable cv_delay;
    std::mutex mtx_delay;

    std::atomic_bool flagActive{false};
    std::unique_ptr<std::thread> thread{nullptr};
    std::function<int(void)> fun;
};

#endif // SMART_THREAD_H
