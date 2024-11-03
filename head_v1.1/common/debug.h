#ifndef DEBUG_H
#define DEBUG_H

#define DBG_INFO "[ ]\t"
#define DBG_ERR "[E]\t"
#define DBG_WARN "[W]\t"

void debug(char *format, ...);
void debug_rf(char *format, ...);
void debug_rx(char x);

#endif // DEBUG_H