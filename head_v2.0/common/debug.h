#ifndef DEBUG_H
#define DEBUG_H

#define DBG_INFO "[ ]\t"
#define DBG_ERR "[E]\t"
#define DBG_WARN "[W]\t"

void debug_uart(const char *format, ...) __attribute__((format(printf, 1, 2)));
void debug_rx(char x);

#endif // DEBUG_H