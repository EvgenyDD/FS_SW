#ifndef EVENT_COUNTER_H
#define EVENT_COUNTER_H

#include <atomic>

class EventCounter
{
public:
    uint32_t get_event_counter() const {return counter;}
    void increment_event_counter() {counter++;}

private:
    std::atomic<uint32_t> counter;
};

#endif // EVENT_COUNTER_H
