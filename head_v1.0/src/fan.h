#ifndef FAN_H
#define FAN_H

#include <stdbool.h>
#include <stdint.h>

bool fan_set(uint8_t fan, uint32_t value);

#endif // FAN_H