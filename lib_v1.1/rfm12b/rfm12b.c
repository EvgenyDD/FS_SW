#include "rfm12b.h"
#include "platform.h"
#include <string.h>

#include "console.h"

#define RFM12B_CMD_RESET 0xFE00
#define RFM12B_CMD_CONFIG 0x8000
#define RFM12B_CMD_POWER 0x8200
#define RFM12B_CMD_FREQ 0xA000
#define RFM12B_CMD_DATA_RATE 0xC600
#define RFM12B_CMD_RX_CTRL 0x9000
#define RFM12B_CMD_DATA_FILTER 0xC228
#define RFM12B_CMD_FIFO_AND_RESET 0xCA00
#define RFM12B_CMD_SYNC_PATTERN 0xCE00
#define RFM12B_CMD_AFC 0xC400
#define RFM12B_CMD_TX_CONFIG 0x9800
#define RFM12B_CMD_MCU_CLK_DIV 0xC000
#define RFM12B_CMD_WAKEUP_TIMER 0xE000
#define RFM12B_CMD_LOW_DUTY 0xC800
#define RFM12B_CMD_TX_WRITE 0xB800
#define RFM12B_CMD_RX_FIFO_READ 0xB000
#define RFM12B_CMD_PWR_TX_0 0x8228 // xtal + TX
#define RFM12B_CMD_PWR_TX_1 0x8238 // xtal + TX + synthesizer
#define RFM12B_CMD_PWR_RX 0x82D8   // xtal + RX + RX baseband + synthesizer
#define RFM12B_CMD_IDLE 0x8208	   // xtal
#define RFM12B_CMD_SLEEP 0x8200

// ===============

#define RFM12B_RX_BW_400KHZ 1
#define RFM12B_RX_BW_340KHZ 2
#define RFM12B_RX_BW_270KHZ 3
#define RFM12B_RX_BW_200KHZ 4
#define RFM12B_RX_BW_134KHZ 5
#define RFM12B_RX_BW_67KHZ 6

#define RFM12B_LNA_GAIN_0DB 0
#define RFM12B_LNA_GAIN_M6DB 1
#define RFM12B_LNA_GAIN_M14DB 2
#define RFM12B_LNA_GAIN_M20DB 3

#define RFM12B_RSSI_THSH_M103 0
#define RFM12B_RSSI_THSH_M97 1
#define RFM12B_RSSI_THSH_M91 2
#define RFM12B_RSSI_THSH_M85 3
#define RFM12B_RSSI_THSH_M79 4
#define RFM12B_RSSI_THSH_M73 5

#define RFM12B_POWER_OUT_0DB 0
#define RFM12B_POWER_OUT_M2_5DB 1
#define RFM12B_POWER_OUT_M5DB 2
#define RFM12B_POWER_OUT_M7_5DB 3
#define RFM12B_POWER_OUT_M10DB 4
#define RFM12B_POWER_OUT_M12_5DB 5
#define RFM12B_POWER_OUT_M15DB 6
#define RFM12B_POWER_OUT_M17_5DB 7

#define DATA_REG_DIS                               \
	RFM12B_CMD_CONFIG |                            \
		(0 << 7) | /* internal data reg disable */ \
		(1 << 6) | /* FIFO mode enable */          \
		(1 << 4) | /* 433 band */                  \
		(7 << 0)   /* 12pF crystal load capacitance */
#define DATA_REG_EN                                \
	RFM12B_CMD_CONFIG |                            \
		(1 << 7) | /* internal data reg disable */ \
		(1 << 6) | /* FIFO mode enable */          \
		(1 << 4) | /* 433 band */                  \
		(7 << 0)   /* 12pF crystal load capacitance */

static volatile bool irq_fired = false;
static bool is_tx = false;
static uint32_t tx_len = 0, tx_ptr = 0;
static uint8_t tx_buf[RFM12B_MAX_PAYLOAD_SIZE + 2];

#define RFM_IRQ_GET() (GPIOC->IDR & (1 << 12))

#define S_WR()                         \
	while(!(SPI3->SR & SPI_FLAG_RXNE)) \
		asm("nop");

#define S_WT()                        \
	while(!(SPI3->SR & SPI_FLAG_TXE)) \
		asm("nop");

#define S_WB()                     \
	while(SPI3->SR & SPI_FLAG_BSY) \
		asm("nop");

#define S_DT(x) *((volatile uint16_t *)&SPI3->DR) = x;
#define S_DR(x) x = (uint16_t)(SPI3->DR);

static volatile uint8_t rf12_buf[RFM12B_MAX_PAYLOAD_SIZE] = {0};
static uint8_t sync_pattern = 0;

void EXTI15_10_IRQHandler(void);

static uint16_t rfm12b_trx_2b(uint16_t word)
{
	uint16_t rx;
	CS_L;
	S_DT(word);
	S_WT();
	S_WR();
	S_DR(rx);
	S_WB();
	CS_H;
	return rx;
}

void rfm12b_reset(void)
{
	rfm12b_trx_2b(0);
	rfm12b_trx_2b(RFM12B_CMD_SLEEP);
	rfm12b_trx_2b(RFM12B_CMD_TX_WRITE);
	while(RFM_IRQ_GET() == 0)
		rfm12b_trx_2b(0);

	delay_ms(150);
}

void rfm12b_config(uint8_t _sync_pattern)
{
	sync_pattern = _sync_pattern;
	rfm12b_trx_2b(DATA_REG_EN);
	rfm12b_trx_2b(RFM12B_CMD_IDLE);
	rfm12b_trx_2b(RFM12B_CMD_FREQ | 1568);		// 433.92 MHz
	rfm12b_trx_2b(RFM12B_CMD_DATA_RATE | 0x08); // 38 kbps
	rfm12b_trx_2b(RFM12B_CMD_RX_CTRL |
				  (1 << 10) | // PIN 16 - VDI
				  (0 << 8) |  // VDI fast
				  (RFM12B_RX_BW_134KHZ << 5) |
				  (RFM12B_LNA_GAIN_0DB << 3) |
				  (RFM12B_RSSI_THSH_M91 << 0));
	rfm12b_trx_2b(RFM12B_CMD_DATA_FILTER |
				  (1 << 7) | // auto clock recovery
				  (1 << 6) | // clock recovery lock control
				  (0 << 4) | // digital filter
				  (4 << 0)); // DQD
	rfm12b_trx_2b(RFM12B_CMD_FIFO_AND_RESET |
				  (8 << 4));
	rfm12b_trx_2b(RFM12B_CMD_FIFO_AND_RESET |
				  (8 << 4) | // FIFO level: 1-15  FIFO gen IT when the number of rx bits reaches this level
				  (0 << 3) | // sync pattern - 2 byte - 0x2Dxx
				  (0 << 2) | // sync patter fifo start cond
				  (1 << 1) | // fifo enable after sync pattern rx
				  (1 << 0)); // non-sensitive to power reset
	rfm12b_trx_2b(RFM12B_CMD_SYNC_PATTERN | sync_pattern);
	rfm12b_trx_2b(RFM12B_CMD_AFC |
				  (2 << 6) | // keep Foffset only in rx (VDI=high)
				  (0 << 4) | // max dev.: no restriction
				  (0 << 3) | // strobe edge
				  (0 << 2) | // high-accuracy mode
				  (1 << 1) | // freq offset register
				  (1 << 0)); //  calc offset freq by AFC
	rfm12b_trx_2b(RFM12B_CMD_TX_CONFIG |
				  (5 << 4) | // FSK out freq deviation - 90kHz
				  RFM12B_POWER_OUT_0DB << 0);
	rfm12b_trx_2b(RFM12B_CMD_WAKEUP_TIMER);
	rfm12b_trx_2b(RFM12B_CMD_LOW_DUTY);
	rfm12b_trx_2b(RFM12B_CMD_MCU_CLK_DIV);

	rfm12b_trx_2b(RFM12B_CMD_PWR_RX);
}

void rfm12b_init(uint8_t _sync_pattern)
{
	rfm12b_hw_init();
	rfm12b_reset();
	rfm12b_config(_sync_pattern);
}

void rfm12b_tx(uint8_t node, const void *data, uint8_t size)
{
	if(size > RFM12B_MAX_PAYLOAD_SIZE) return;
	while(tx_ptr != tx_len)
		rfm12b_poll_irq();
	tx_len = 1 + size + 1 /* dummy */;
	tx_ptr = 0;
	tx_buf[0] = node;
	is_tx = 1;
	memcpy(tx_buf + 1, data, size);
	// _console_print("OPEN TX\n");

	rfm12b_trx_2b(DATA_REG_EN);
	rfm12b_trx_2b(RFM12B_CMD_PWR_TX_0);
	for(volatile uint32_t i = 0; i < 32; i++)
		asm("nop");
	rfm12b_trx_2b(RFM12B_CMD_PWR_TX_1); // bytes will be fed via interrupts
}

uint32_t irq_count = 0, last_sts = 0;

void rfm12b_irq_handler(void)
{
	irq_count++;
	uint16_t sts = rfm12b_trx_2b(0);
	last_sts = sts;
	// _console_print("x%x ", sts);
	if(is_tx)
	{
		if(tx_ptr >= tx_len)
		{
			// _console_print("F\n");
			is_tx = false;
			rfm12b_trx_2b(RFM12B_CMD_IDLE);
			rfm12b_trx_2b(DATA_REG_DIS);
		}
		else
		{
			// _console_print("t\n");
			rfm12b_trx_2b(RFM12B_CMD_TX_WRITE | tx_buf[tx_ptr++]);
		}
		// rfm12b_trx_2b(RFM12B_CMD_IDLE);
		// rfm12b_trx_2b(RFM12B_CMD_PWR_RX);
	}
	else
	{
		// _console_print("r\n");
		uint8_t in = rfm12b_trx_2b(RFM12B_CMD_RX_FIFO_READ);
		_console_print("%d ", in);
		rfm12b_trx_2b(RFM12B_CMD_FIFO_AND_RESET |
					  (8 << 4));
		rfm12b_trx_2b(RFM12B_CMD_FIFO_AND_RESET |
					  (8 << 4) | // FIFO level: 1-15  FIFO gen IT when the number of rx bits reaches this level
					  (0 << 3) | // sync pattern - 2 byte - 0x2Dxx
					  (0 << 2) | // sync patter fifo start cond
					  (1 << 1) | // fifo enable after sync pattern rx
					  (1 << 0)); // non-sensitive to power reset
	}
}

void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line12) == SET)
	{
		EXTI_ClearITPendingBit(EXTI_Line12);
		irq_fired = 1;
	}
}

void rfm12b_poll_irq(void)
{
	if(irq_fired)
	{
		irq_fired = false;
		rfm12b_irq_handler();
	}
}

void rfm12b_poll(uint32_t diff_ms)
{
	rfm12b_poll_irq();
}