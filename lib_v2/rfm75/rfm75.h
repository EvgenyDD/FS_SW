#ifndef RFM75_H__
#define RFM75_H__

#include "rfm75_hal.h"
#include <stdint.h>

#define RFM75_CFG_DELAY_US 8000

#define RFM75_MAX_PAYLOAD_SIZE 32
#define RFM75_MAX_NODES 6

#define RFM75_CMD_R_REG 0x00
#define RFM75_CMD_W_REG 0x20
#define RFM75_CMD_R_RX_PL 0x61
#define RFM75_CMD_W_TX_PL 0xA0
#define RFM75_CMD_FLUSH_TX 0xE1
#define RFM75_CMD_FLUSH_RX 0xE2
#define RFM75_CMD_REUSE_TX_PL 0xE3
#define RFM75_CMD_W_TX_PL_NOACK 0xB0
#define RFM75_CMD_W_ACK_PL 0xA8
#define RFM75_CMD_ACTIVATE 0x50
#define RFM75_CMD_R_RX_PL_WID 0x60
#define RFM75_CMD_NOP 0xFF

#define RFM75_REG_CONFIG 0x00
#define RFM75_REG_EN_AA 0x01
#define RFM75_REG_EN_RXADDR 0x02
#define RFM75_REG_SETUP_AW 0x03
#define RFM75_REG_SETUP_RETR 0x04
#define RFM75_REG_RF_CH 0x05
#define RFM75_REG_RF_SETUP 0x06
#define RFM75_REG_STATUS 0x07
#define RFM75_REG_OBSERVE_TX 0x08
#define RFM75_REG_CD 0x09
#define RFM75_REG_RX_ADDR_P0 0x0A
#define RFM75_REG_RX_ADDR_P1 0x0B
#define RFM75_REG_RX_ADDR_P2 0x0C
#define RFM75_REG_RX_ADDR_P3 0x0D
#define RFM75_REG_RX_ADDR_P4 0x0E
#define RFM75_REG_RX_ADDR_P5 0x0F
#define RFM75_REG_TX_ADDR 0x10
#define RFM75_REG_RX_PW_P0 0x11
#define RFM75_REG_RX_PW_P1 0x12
#define RFM75_REG_RX_PW_P2 0x13
#define RFM75_REG_RX_PW_P3 0x14
#define RFM75_REG_RX_PW_P4 0x15
#define RFM75_REG_RX_PW_P5 0x16
#define RFM75_REG_FIFO_STATUS 0x17
#define RFM75_REG_DYN_PL 0x1C
#define RFM75_REG_FEATURE 0x1D

// interrupt status
#define RFM75_STS_RBANK (1 << 7)
#define RFM75_STS_RX_DR (1 << 6)
#define RFM75_STS_TX_DS (1 << 5)
#define RFM75_STS_MAX_RT (1 << 4)
#define RFM75_STS_TX_FULL (1 << 0)

#define RFM75_FIFO_STATUS_TX_REUSE 0x40
#define RFM75_FIFO_STATUS_TX_FULL 0x20
#define RFM75_FIFO_STATUS_TX_EMPTY 0x10
#define RFM75_FIFO_STATUS_RX_FULL 0x02
#define RFM75_FIFO_STATUS_RX_EMPTY 0x01

#define RFM75_RETR_DELAY_250us (0 << 4)
#define RFM75_RETR_DELAY_500us (1 << 4)
#define RFM75_RETR_DELAY_750us (2 << 4)
#define RFM75_RETR_DELAY_1000us (3 << 4)
#define RFM75_RETR_DELAY_1250us (4 << 4)
#define RFM75_RETR_DELAY_1500us (5 << 4)
#define RFM75_RETR_DELAY_1750us (6 << 4)
#define RFM75_RETR_DELAY_2000us (7 << 4)
#define RFM75_RETR_DELAY_2250us (8 << 4)
#define RFM75_RETR_DELAY_2500us (9 << 4)
#define RFM75_RETR_DELAY_2750us (10 << 4)
#define RFM75_RETR_DELAY_3000us (11 << 4)
#define RFM75_RETR_DELAY_3250us (12 << 4)
#define RFM75_RETR_DELAY_3500us (13 << 4)
#define RFM75_RETR_DELAY_3750us (14 << 4)
#define RFM75_RETR_DELAY_4000us (15 << 4)

void rfm75_init(void);
void rfm75_poll_irq(void);
void rfm75_poll(uint32_t diff_ms);
void rfm75_interrupt(void);
void rmf75_delay_packets(uint32_t time_ms);
void rfm75_tx(uint8_t dev_idx, const uint8_t *data, uint32_t size);
bool rfm75_is_delayed(void);
void rfm75_tx_force(uint8_t dev_idx, const uint8_t *data, uint32_t size);

extern void rfm75_process_data(uint8_t dev_idx, const uint8_t *data, uint8_t data_len);
extern void rfm75_tx_done(void);
extern void rfm75_tx_fail(uint8_t lost, uint8_t rt);

extern uint8_t rfm75_node_addresses[RFM75_MAX_NODES][5];

#endif // RFM75_H__