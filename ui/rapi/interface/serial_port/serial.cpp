#include <algorithm>

#if !defined(_WIN32) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
#include <alloca.h>
#endif

//#if defined (__MINGW32__)
//# define alloca __builtin_alloca
//#endif

#include "serial.hpp"
#include "impl.hpp"

using std::invalid_argument;
using std::min;
using std::numeric_limits;
using std::size_t;
using std::string;
using std::vector;

using serial::bytesize_t;
using serial::flowcontrol_t;
using serial::IOException;
using serial::parity_t;
using serial::Serial;
using serial::SerialException;
using serial::stopbits_t;

class Serial::ScopedReadLock
{
public:
    ScopedReadLock(SerialImpl *pimpl) : pimpl_(pimpl)
    {
        this->pimpl_->readLock();
    }
    ~ScopedReadLock()
    {
        this->pimpl_->readUnlock();
    }

private:
    // Disable copy constructors
    ScopedReadLock(const ScopedReadLock &);
    const ScopedReadLock &operator=(ScopedReadLock);

    SerialImpl *pimpl_;
};

class Serial::ScopedWriteLock
{
public:
    ScopedWriteLock(SerialImpl *pimpl) : pimpl_(pimpl)
    {
        this->pimpl_->writeLock();
    }
    ~ScopedWriteLock()
    {
        this->pimpl_->writeUnlock();
    }

private:
    // Disable copy constructors
    ScopedWriteLock(const ScopedWriteLock &);
    const ScopedWriteLock &operator=(ScopedWriteLock);
    SerialImpl *pimpl_;
};

Serial::Serial(const string &port, uint32_t baudrate, serial::Timeout timeout,
               bytesize_t bytesize, parity_t parity, stopbits_t stopbits,
               flowcontrol_t flowcontrol)
    : pimpl_(new SerialImpl(port, baudrate, bytesize, parity,
                            stopbits, flowcontrol))
{
    pimpl_->setTimeout(timeout);
}

Serial::~Serial() { delete pimpl_; }
void Serial::open() { pimpl_->open(); }
void Serial::close() { pimpl_->close(); }
bool Serial::isOpen() const { return pimpl_->isOpen(); }
size_t Serial::available() { return pimpl_->available(); }

bool Serial::waitReadable()
{
    serial::Timeout timeout(pimpl_->getTimeout());
    return pimpl_->waitReadable(timeout.read_timeout_constant);
}

void Serial::waitByteTimes(size_t count)
{
    pimpl_->waitByteTimes(count);
}

size_t Serial::read(std::vector<uint8_t> &buffer)
{
    ScopedReadLock lock(this->pimpl_);

    size_t bytes_read = this->pimpl_->read(buffer);

    return bytes_read;
}

size_t Serial::write(const std::vector<uint8_t> &data)
{
    ScopedReadLock lock(this->pimpl_);
//    ScopedWriteLock lock(this->pimpl_);
    return pimpl_->write(&data[0], data.size());
}

void Serial::setPort(const string &port)
{
    ScopedReadLock rlock(this->pimpl_);
    ScopedWriteLock wlock(this->pimpl_);
    bool was_open = pimpl_->isOpen();
    if(was_open)
        close();
    pimpl_->setPort(port);
    if(was_open)
        open();
}

string Serial::getPort() const
{
    return pimpl_->getPort();
}

void Serial::flush()
{
    ScopedReadLock rlock(this->pimpl_);
    ScopedWriteLock wlock(this->pimpl_);
    pimpl_->flush();
}

void Serial::flushInput()
{
    ScopedReadLock lock(this->pimpl_);
    pimpl_->flushInput();
}

void Serial::flushOutput()
{
    ScopedWriteLock lock(this->pimpl_);
    pimpl_->flushOutput();
}
