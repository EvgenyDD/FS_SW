#ifndef DEBUG_TOOLS_H
#define DEBUG_TOOLS_H

#include <iostream>
#include <atomic>
#include <mutex>
#include "date.h"

class DebugVarProvider
{
public:
    static auto &get_print_mtx()
    {
        static std::mutex print_mtx;
        return print_mtx;
    }

    static std::atomic_int &get_ref_cnt()
    {
        static std::atomic_int class_ref_cnt;
        return class_ref_cnt;
    }
};

#if 1
#define TS date::format("%F %T: ", std::chrono::system_clock::now())
#else
#define TS ""
#endif

#define PRINT_CTOR() //std::cout << TS << "    [CTOR" << DebugVarProvider::get_ref_cnt()++ << "]\t" << __FUNCTION__ << std::endl;
#define PRINT_DTOR() //std::cout << TS << "    [DTOR" << --DebugVarProvider::get_ref_cnt() << "]\t" << __FUNCTION__ << std::endl;
#define PRINT_DTOR_PREPARE() //std::cout << TS << "    [dtor" << DebugVarProvider::get_ref_cnt()-1 << "]\t" << __FUNCTION__ << std::endl;
//#define PRINT_INIT(msg) std::cout << TS << "[INIT]\t" << __FUNCTION__ << msg << std::endl;
#define PRINT_MSG(msg) std::cout << TS << "\t" << msg << std::endl;
#define PRINT_MSG_(msg) std::cout << msg << std::endl;
#define PRINT_MSG_LOCK(msg) {std::unique_lock<std::mutex> lck(DebugVarProvider::get_print_mtx()); std::cout << TS << "\t" << msg << std::endl;}
#define PRINT_ERROR(msg) std::cout << TS << "[ERROR]\t" << msg << std::endl;
#define PRINT_WARNING(msg) std::cout << TS << "[WARNING]\t" << msg << std::endl;
#define PRINT_INFO(msg) std::cout << TS << msg << std::endl;

#endif // DEBUG_TOOLS_H
