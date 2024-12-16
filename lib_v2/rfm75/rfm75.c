#include "rfm75.h"
#include "platform.h"
#include <string.h>

#define CFG_RF_CHANNEL 84
#define CFG_RX_PKT_SZ 8
#define CFG_TX_PKT_SZ 64

static volatile bool is_sending = false;
static volatile bool irq_fired = false;
static uint32_t delay_post_ms = 0;

struct
{
	uint8_t data[RFM75_MAX_PAYLOAD_SIZE];
	uint8_t size;
	uint8_t pipe;
} rx_pkts[CFG_RX_PKT_SZ];

struct
{
	uint8_t data[RFM75_MAX_PAYLOAD_SIZE];
	uint8_t size;
	uint8_t pipe;
} tx_pkts[CFG_TX_PKT_SZ];
static uint32_t rx_pkts_size = 0, tx_pkts_push = 0, tx_pkts_pop = 0;

static uint8_t spi_rw(uint8_t byte)
{
	uint8_t rx;
	S_WB();
	S_DT(byte);
	S_WT();
	S_WR();
	S_DR(rx);
	return rx;
}

static void cmd_buff_read(uint8_t cmd, uint8_t *data, uint8_t len)
{
	CS_L;
	spi_rw(cmd);
	for(uint8_t i = 0; i < len; i++)
		data[i] = spi_rw(0);
	CS_H;
}

static void cmd_buff_read_dummy(uint8_t cmd, uint8_t len)
{
	CS_L;
	spi_rw(cmd);
	for(uint8_t i = 0; i < len; i++)
		spi_rw(0);
	CS_H;
}

static uint8_t cmd_read(uint8_t cmd)
{
	CS_L;
	spi_rw(cmd);
	uint8_t tmp = spi_rw(0);
	CS_H;
	return tmp;
}

static void cmd(uint8_t cmd)
{
	CS_L;
	spi_rw(cmd);
	CS_H;
}

static void cmd_write(uint8_t cmd, uint8_t data)
{
	CS_L;
	spi_rw(cmd);
	spi_rw(data);
	CS_H;
}

static void cmd_buff_write(uint8_t cmd, const uint8_t *data, uint8_t data_len)
{
	CS_L;
	spi_rw(cmd);
	for(uint8_t i = 0; i < data_len; i++)
		spi_rw(data[i]);
	CS_H;
}

static uint8_t reg_read(uint8_t cmd)
{
	CS_L;
	spi_rw(RFM75_CMD_R_REG | (cmd & 0x1F));
	uint8_t recv = spi_rw(RFM75_CMD_NOP);
	CS_H;
	return recv;
}

static void reg_write(uint8_t reg, uint8_t data) { cmd_write(RFM75_CMD_W_REG | (reg & 0x1F), data); }
static void reg_buff_write(uint8_t reg, const uint8_t *data, uint8_t data_len) { cmd_buff_write(RFM75_CMD_W_REG | (reg & 0x1F), data, data_len); }

///////////////

static uint8_t get_sts(void)
{
	CS_L;
	uint8_t recv = spi_rw(RFM75_CMD_NOP);
	CS_H;
	return recv;
}

static void select_bank(uint8_t bank)
{
	uint8_t currbank = get_sts() & RFM75_STS_RBANK;
	if((currbank != 0 && bank == 0) || (currbank == 0 && bank != 0)) cmd_write(RFM75_CMD_ACTIVATE, 0x53);
}

// static uint8_t rfm75_tx_fifo_full(void) { return (reg_read(RFM75_REG_FIFO_STATUS) & RFM75_FIFO_STATUS_TX_FULL) != 0; }
// static uint8_t rfm75_tx_fifo_empty(void) { return (reg_read(RFM75_REG_FIFO_STATUS) & RFM75_FIFO_STATUS_TX_EMPTY) != 0; }
// static uint8_t rfm75_rx_fifo_full(void) { return (reg_read(RFM75_REG_FIFO_STATUS) & RFM75_FIFO_STATUS_RX_FULL) != 0; }
// static uint8_t rfm75_rx_fifo_empty(void) { return (reg_read(RFM75_REG_FIFO_STATUS) & RFM75_FIFO_STATUS_RX_EMPTY) != 0; }

static uint8_t rx_next_pipe(void) { return (get_sts() >> 1) & 0x07; }
// static uint8_t rx_next_pipe_sts(uint8_t last_sts) { return (last_sts >> 1) & 0x07; }
static uint8_t rx_next_size(void) { return cmd_read(RFM75_CMD_R_RX_PL_WID); }

static uint8_t chip_is_present(void)
{
	uint8_t tmp1 = get_sts();
	cmd_write(RFM75_CMD_ACTIVATE, 0x53);
	uint8_t tmp2 = get_sts();
	cmd_write(RFM75_CMD_ACTIVATE, 0x53);
	return (tmp1 ^ tmp2) == 0x80;
}

static void enter_rx(void)
{
	CE_DEACT;
	select_bank(0);
	reg_write(RFM75_REG_CONFIG, 0x3F);
	reg_write(RFM75_REG_STATUS, RFM75_STS_RX_DR | RFM75_STS_TX_DS | RFM75_STS_MAX_RT);
	CE_ACT;
}

void rfm75_poll_irq(void)
{
	if(irq_fired)
	{
		rfm75_delay_trigger();
		uint8_t sts = get_sts();
		if(sts & RFM75_STS_TX_DS) // TX
		{
			reg_write(RFM75_REG_STATUS, RFM75_STS_TX_DS);
			is_sending = false;
			rfm75_tx_done();
			enter_rx();
		}

		if(sts & RFM75_STS_MAX_RT) // TX retransmit limit
		{
			uint8_t stat = reg_read(RFM75_REG_OBSERVE_TX);

			reg_write(RFM75_REG_STATUS, RFM75_STS_MAX_RT);
			is_sending = false;
			rfm75_tx_fail(stat >> 4, stat & 0x0F);
			enter_rx();
		}

		if(sts & RFM75_STS_RX_DR) // RX
		{
			for(;;)
			{
				uint8_t pipe = rx_next_pipe();
				if(pipe == 0x07) break;
				uint8_t size = rx_next_size();
				if(rx_pkts_size < CFG_RX_PKT_SZ)
				{
					rx_pkts[rx_pkts_size].pipe = pipe;
					rx_pkts[rx_pkts_size].size = size;
					cmd_buff_read(RFM75_CMD_R_RX_PL, rx_pkts[rx_pkts_size].data, size);
					rx_pkts_size++;
				}
				else
				{
					cmd_buff_read_dummy(RFM75_CMD_R_RX_PL, size);
				}
			}
			reg_write(RFM75_REG_STATUS, RFM75_STS_RX_DR);
			CE_ACT;
		}

		irq_fired = 0;
	}
}

void rfm75_interrupt(void) { irq_fired = 1; }
bool rfm75_is_delayed(void) { return delay_post_ms != 0; }
void rmf75_delay_packets(uint32_t time_ms)
{
	if(delay_post_ms < time_ms) delay_post_ms = time_ms;
}

void rfm75_tx(uint8_t dev_idx, const uint8_t *data, uint32_t size)
{
	if(delay_post_ms == 0)
	{
		rfm75_tx_force(dev_idx, data, size);
	}
	else
	{
		tx_pkts[tx_pkts_push].pipe = dev_idx;
		tx_pkts[tx_pkts_push].size = size;
		memcpy(tx_pkts[tx_pkts_push].data, data, size);
		tx_pkts_push = (tx_pkts_push + 1) % (CFG_TX_PKT_SZ);
	}
}

void rfm75_tx_force(uint8_t dev_idx, const uint8_t *data, uint32_t size)
{
	rfm75_delay_tx();
	while(is_sending && rfm75_delay_expired() == false)
	{
		rfm75_poll_irq();
	}
	is_sending = true;

	CE_DEACT;
	select_bank(0);
	reg_buff_write(RFM75_REG_TX_ADDR, rfm75_node_addresses[dev_idx], 5);
	reg_write(RFM75_REG_CONFIG, 0x4E);
	reg_write(RFM75_REG_STATUS, RFM75_STS_RX_DR | RFM75_STS_TX_DS | RFM75_STS_MAX_RT);

	// cmd(RFM75_CMD_FLUSH_RX);
	cmd(RFM75_CMD_FLUSH_TX);

	cmd_buff_write(RFM75_CMD_W_TX_PL, data, size);
	CE_ACT;
}

void rfm75_init(void)
{
	rfm75_hal_hw_init_1();

	select_bank(0);
	reg_write(RFM75_REG_CONFIG, 0);

	while(!chip_is_present())
		;
	delay_ms(2);

	select_bank(0);

	reg_write(RFM75_REG_FEATURE, 0x01);									  // check if "feature register" is writable before (de)activating it
	if(!reg_read(RFM75_REG_FEATURE)) cmd_write(RFM75_CMD_ACTIVATE, 0x73); // activate feature register if not activated

	static const uint8_t init_regs[][2] = {
		{RFM75_REG_CONFIG, (1 << 3) |					// en CRC
							   (1 << 2) |				// CRC - 1/0: 2 byte/1byte
							   (1 << 1) |				// power up
							   (1 << 0)},				// PRX
		{RFM75_REG_EN_AA, 0x3F},						// auto ack all pipes
		{RFM75_REG_EN_RXADDR, (1 << 0) |				// enable RX pipe 0
								  (0 << 1) |			// RX pipe 1
								  (0 << 2) |			// RX pipe 2
								  (0 << 3) |			// RX pipe 3
								  (0 << 4) |			// RX pipe 4
								  (0 << 5)},			// RX pipe 5
		{RFM75_REG_SETUP_AW, 3},						// address width 1/2/3: 3/4/5 byte
		{RFM75_REG_SETUP_RETR, RFM75_RETR_DELAY_250us | // retransmit delay
								   8},					// retransmit count
		{RFM75_REG_RF_CH, CFG_RF_CHANNEL},				// 2400MHz + value (7bit)
		{RFM75_REG_RF_SETUP, (0 << 3) |					// 1Mbps
								 (3 << 1) |				// RF power
								 (1 << 0)},				// LNA high gain
		{RFM75_REG_STATUS, 0x70},						// Clear interrupt flags
		{RFM75_REG_OBSERVE_TX, 0},						//
		{RFM75_REG_CD, 0},								//
		{RFM75_REG_RX_PW_P0, 0},						//
		{RFM75_REG_RX_PW_P1, 0},						//
		{RFM75_REG_RX_PW_P2, 0},						//
		{RFM75_REG_RX_PW_P3, 0},						//
		{RFM75_REG_RX_PW_P4, 0},						//
		{RFM75_REG_RX_PW_P5, 0},						//
		{RFM75_REG_FIFO_STATUS, 0},						//
		{RFM75_REG_DYN_PL, 0x3F},						// dynamic payload all pipes
		{RFM75_REG_FEATURE, (1 << 2) |					// enable dynamic payload length
								(1 << 1) |				// enable PL with ACK
								(1 << 0)}				// enable W_TX_PAYLOAD_NOACK command
	};

	for(uint8_t i = 0; i < sizeof(init_regs) / sizeof(init_regs[0]); i++)
	{
		reg_write(init_regs[i][0], init_regs[i][1]);
		if(reg_read(init_regs[i][0]) != init_regs[i][1])
		{
			if(init_regs[i][0] != RFM75_REG_STATUS &&
			   init_regs[i][0] != RFM75_REG_FIFO_STATUS)
				while(1)
				{
				}
		}
	}

	for(uint32_t i = 0; i < RFM75_MAX_NODES; i++)
		reg_buff_write(RFM75_REG_RX_ADDR_P0 + i, rfm75_node_addresses[i], 5);

	select_bank(1);
#if 1
	reg_buff_write(0x00, (uint8_t[]){0x40, 0x4B, 0x01, 0xE2}, 4);
	reg_buff_write(0x01, (uint8_t[]){0xC0, 0x4B, 0x00, 0x00}, 4);
	reg_buff_write(0x02, (uint8_t[]){0xD0, 0xFC, 0x8C, 0x02}, 4);
	reg_buff_write(0x03, (uint8_t[]){0x99, 0x00, 0x39, 0x21}, 4);
	reg_buff_write(0x04, (uint8_t[]){0xF9, 0x96, 0x82, 0x1B}, 4);
	reg_buff_write(0x05, (uint8_t[]){0x24, 0x02, 0x0F, 0xA6}, 4);
#else
	reg_buff_write(0x00, (uint8_t[]){0xE2, 0x01, 0x4B, 0x40}, 4);
	reg_buff_write(0x01, (uint8_t[]){0x00, 0x00, 0x4B, 0xC0}, 4);
	reg_buff_write(0x02, (uint8_t[]){0x02, 0x8C, 0xFC, 0xD0}, 4);
	reg_buff_write(0x03, (uint8_t[]){0x21, 0x39, 0x00, 0x99}, 4);
	reg_buff_write(0x04, (uint8_t[]){0x1B, 0x82, 0x96, 0xF9}, 4);
	reg_buff_write(0x05, (uint8_t[]){0xA6, 0x0F, 0x02, 0x24}, 4);
#endif
	reg_buff_write(0x06, (uint8_t[]){0, 0, 0, 0}, 4);
	reg_buff_write(0x07, (uint8_t[]){0, 0, 0, 0}, 4);
	reg_buff_write(0x08, (uint8_t[]){0, 0, 0, 0}, 4);
	reg_buff_write(0x09, (uint8_t[]){0, 0, 0, 0}, 4);
	reg_buff_write(0x0A, (uint8_t[]){0, 0, 0, 0}, 4);
	reg_buff_write(0x0B, (uint8_t[]){0, 0, 0, 0}, 4);
	reg_buff_write(0x0C, (uint8_t[]){0x00, 0x12, 0x73, 0x05}, 4);
	reg_buff_write(0x0D, (uint8_t[]){0x36, 0xB4, 0x80, 0x00}, 4);
	reg_buff_write(0x0E, (uint8_t[]){0x41, 0x20, 0x08, 0x04, 0x81, 0x20, 0xCF, 0xF7, 0xFE, 0xFF, 0xFF}, 11);

	delay_ms(2);

	select_bank(1);
	reg_buff_write(0x04, (uint8_t[]){0xF9 | 0x06, 0x96, 0x82, 0x1B}, 4);
	delay_ms(2);
	reg_buff_write(0x04, (uint8_t[]){0xF9 | 0x00, 0x96, 0x82, 0x1B}, 4);
	select_bank(0);

	rfm75_hal_hw_init_2();

	cmd(RFM75_CMD_FLUSH_RX);
	cmd(RFM75_CMD_FLUSH_TX);

	enter_rx();
}

void rfm75_poll(uint32_t diff_ms)
{
	rfm75_poll_irq();
	for(uint32_t i = 0; i < rx_pkts_size; i++)
	{
		if(rx_pkts[i].size) rfm75_process_data(rx_pkts[i].pipe, rx_pkts[i].data, rx_pkts[i].size);
	}
	rx_pkts_size = 0;
	if(rfm75_is_delayed() == false)
	{
		while(tx_pkts_pop != tx_pkts_push)
		{
			rfm75_tx_force(tx_pkts[tx_pkts_pop].pipe, tx_pkts[tx_pkts_pop].data, tx_pkts[tx_pkts_pop].size);
			tx_pkts_pop = (tx_pkts_pop + 1) % (CFG_TX_PKT_SZ);
		}
	}
	delay_post_ms = delay_post_ms <= diff_ms ? 0 : delay_post_ms - diff_ms;
}
