#include "debug.h"
#include "air_protocol.h"
#include "console.h"
#include "platform.h"
#include "rfm75.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CON_OUT_BUF_SZ 512
#define CON_IN_BUF_SZ 512

static char buffer_tx[CON_OUT_BUF_SZ + 1];
static char buffer_rx[CON_IN_BUF_SZ];
static int buffer_rx_cnt = 0;
static char buffer_rx_rf[60 + 1];
static volatile bool is_tx = false;

void debug_uart(const char *format, ...)
{
	if(is_tx) return;
	is_tx = true;

	va_list ap;
	va_start(ap, format);
	int len = vsnprintf(buffer_tx, CON_OUT_BUF_SZ, format, ap);
	va_end(ap);

	for(int i = 0; i < len; i++)
	{
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
		{
		}
		USART_SendData(USART3, buffer_tx[i]);
	}
	is_tx = false;
}

void debug_rx(char x)
{
	if(x == '\n')
	{
		if(buffer_rx_cnt < (int)sizeof(buffer_rx) + 1)
		{
			buffer_rx[buffer_rx_cnt] = x;
			buffer_rx[buffer_rx_cnt + 1] = '\0';
			console_cb(buffer_rx, buffer_rx_cnt);
		}
		buffer_rx_cnt = 0;
	}
	else
	{
		if(buffer_rx_cnt >= (int)sizeof(buffer_rx)) buffer_rx_cnt = 0;
		buffer_rx[buffer_rx_cnt] = x;
		buffer_rx_cnt++;
	}
}
