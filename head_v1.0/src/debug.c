#include "debug.h"
#include "main.h"
#include "air_protocol.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const uint8_t iterator_ctrl;
extern UART_HandleTypeDef huart3;

#define CON_OUT_BUF_SZ 512
#define CON_IN_BUF_SZ 512

char buffer_rx[CON_IN_BUF_SZ];
int buffer_rx_cnt = 0;

static volatile bool is_tx = false;

void debug_parse(char *s);

void debug_init(void)
{
    __HAL_UART_ENABLE(&huart3);
}

void debug(char *format, ...)
{
    if(is_tx) return;
    is_tx = true;

    static char buffer[CON_OUT_BUF_SZ + 1];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buffer, CON_OUT_BUF_SZ, format, ap);
    va_end(ap);

    HAL_UART_Transmit(&huart3, buffer, strlen(buffer), 200);

    is_tx = false;
}

void debug_rf(char *format, ...)
{
    if(is_tx) return;
    is_tx = true;

    static char buffer[60 + 1] = "\x28";
    va_list ap;

    va_start(ap, format);
    vsnprintf(buffer+1, sizeof(buffer)-1, format, ap);
    va_end(ap);

    air_protocol_send_async(iterator_ctrl, buffer, strlen(buffer));

    is_tx = false;
}

void debug_rx(char x)
{
    if(x == '\n')
    {
        buffer_rx[buffer_rx_cnt] = x;
        buffer_rx[buffer_rx_cnt + 1] = '\0';
        debug_parse(buffer_rx);
        buffer_rx_cnt = 0;
    }
    else
    {
        buffer_rx[buffer_rx_cnt] = x;
        buffer_rx_cnt++;
    }
}
