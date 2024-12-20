#ifndef RFM12B_H
#define RFM12B_H

#include <stdbool.h>
#include <stdint.h>

#define RFM12B_MAX_PAYLOAD_SIZE 128

#define CS_H                                 \
	for(volatile uint32_t i = 0; i < 8; i++) \
		asm("nop");                          \
	GPIOD->BSRRL = (1 << 2);                 \
	for(volatile uint32_t i = 0; i < 8; i++) \
		asm("nop");

#define CS_L                                 \
	GPIOD->BSRRH = (1 << 2);                 \
	for(volatile uint32_t i = 0; i < 8; i++) \
		asm("nop");

void rfm12b_init(uint8_t sync_pattern);
void rfm12b_hw_init(void);
void rfm12b_irq_handler(void);

void rfm12b_poll_irq(void);
void rfm12b_poll(uint32_t diff_ms);

void rfm12b_reset(void);
void rfm12b_config(uint8_t sync_pattern);

void rfm12b_tx(uint8_t node, const void *data, uint8_t size);

extern void rfm12b_process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len);

#endif // RFM12B_H