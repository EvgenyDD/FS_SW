#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <functional>

#include "serial.hpp"
#include "../abstract_interface.h"
#include "parsers/abstract_parser.h"

class SerialInterface : public AbstractInterface
{
public:
    SerialInterface(bool &success_init, int id, uint32_t interfaceTimeoutMs, std::string &&connectId, std::string &&name) :
        AbstractInterface(
            id,
            interfaceTimeoutMs,
            std::forward<std::string>(connectId),
            std::forward<std::string>(name))
    {
        PRINT_CTOR();

        {
            std::unique_lock<std::shared_timed_mutex> lock(if_impl_mutex);
            p_impl.reset(new AbstractParser(id, if_impl_mutex));
        }

        try
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // to stabilize connection
            serialPort.reset(new serial::Serial(get_connect_id()/*, 9600, serial::Timeout::simpleTimeout(0)*/));

            success_init = true;
        }
        catch(const std::exception &exc)
        {
            interface_fault_message = exc.what();
            interface_is_valid = false;
            success_init = false;

            PRINT_INFO("[X]\t" << get_fault_message());
        }

        if(interface_is_valid)
        {
            //            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            thread_receive.start(std::bind(&SerialInterface::threadReceive, this));

            //            auto x = serialPort->getTimeout();

            //            PRINT_MSG("TO: " << x.inter_byte_timeout);
            //            PRINT_MSG("TO: " << x.read_timeout_constant);
            //            PRINT_MSG("TO: " << x.write_timeout_constant);
            //            PRINT_MSG("TO: " << x.read_timeout_multiplier);
            //            PRINT_MSG("TO: " << x.write_timeout_multiplier);
            //            serialPort->setTimeout(x);

            //            std::vector<uint8_t> data;
            //            data.resize(20);
            //            uint32_t var = 511;
            //            data[0] = (uint8_t)var;
            //            data[1] = (uint8_t)(var >> 8U);
            //            var = 5000;
            //            data[2] = (uint8_t)var;
            //            data[3] = (uint8_t)(var >> 8U);
            //            tx_data(data);
        }
    }

    virtual ~SerialInterface()
    {
        PRINT_DTOR_PREPARE();

        thread_receive.terminate();

        {
            std::unique_lock<std::shared_timed_mutex> lock(if_impl_mutex);
            p_impl.reset();
        }

        if(interface_is_valid)
            serialPort->close();

        PRINT_DTOR();
    }

    ErrorCodes tx_data(const std::vector<uint8_t> &data)
    {
        try
        {
            serialPort->write(data);
        }
        catch(const std::exception &exc)
        {
            interface_fault_message = exc.what();
            interface_is_valid = false;
            return ErrorCodes::GenericError;
        }
        return ErrorCodes::Ok;
    }

private:
    std::vector<uint8_t> buffer;
    int threadReceive()
    {
        try
        {
            serialPort->read(buffer);

            if(buffer.size())
            {
                p_impl->rx_data(buffer);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        catch(const std::exception &exc)
        {
            PRINT_ERROR("FAULT@!" << exc.what());
            interface_fault_message = exc.what();
            interface_is_valid = false;

            return 1;
        }
        return 0;
    }

    std::unique_ptr<serial::Serial> serialPort;
};

#endif // SERIAL_INTERFACE_H
