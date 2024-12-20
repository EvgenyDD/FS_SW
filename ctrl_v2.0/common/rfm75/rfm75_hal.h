#ifndef RFM75_HAL_H__
#define RFM75_HAL_H__

#include <stdbool.h>

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

#define CE_ACT GPIOA->BSRRL = (1 << 2)
#define CE_DEACT GPIOA->BSRRH = (1 << 2)

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

void EXTI0_IRQHandler(void);

void rfm75_hal_hw_init_1(void);
void rfm75_hal_hw_init_2(void);

void rfm75_delay_tx(void);
void rfm75_delay_trigger(void);
bool rfm75_delay_expired(void);

#endif // RFM75_HAL_H__