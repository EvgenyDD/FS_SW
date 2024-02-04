#ifndef DEBUG_H
#define DEBUG_H

#include "debug_common.h"

void debug_init(void);

void debug(char *format, ...);
void debug_rx(char x);

void debug_disable_usb(void);

#endif // DEBUG_H