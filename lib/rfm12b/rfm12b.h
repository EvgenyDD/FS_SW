#ifndef RFM12B_H
#define RFM12B_H

#include <stdbool.h>
#include <stdint.h>

/// RF12 Maximum message size in bytes.
#define RFM12B_MAXDATA 128

void rfm12b_init(uint8_t sync_pattern);
void rfm12b_irq_handler(void);

void rfm12b_reset(void);
void rfm12b_config(uint8_t sync_pattern);

// crypt
void rfm12b_encrypt(const uint8_t *key, uint8_t key_len);

// power management
void rfm12b_sleep(void);
void rfm12b_wakeup(void);

// tx
void rfm12b_send(uint8_t to_node_id, const void *data, uint8_t data_len, bool request_ack, bool is_sleep);
void rfm12b_sendACK(const void *data, uint8_t data_len, bool is_sleep);

// rx
bool rfm12b_rx_complete(void);

// getters
bool rfm12b_is_crc_pass(void);
bool rfm12b_is_ack_requested(void);
bool rfm12b_is_ack_received(uint8_t drom_node_id);

uint8_t rfm12b_get_data_len(void);
volatile uint8_t *rfm12b_get_data(void);
uint8_t rfm12b_get_sender_id(void);
uint8_t rfm12b_get_dest_id(void);
uint8_t rfm12b_get_own_id(void);

#endif // RFM12B_H