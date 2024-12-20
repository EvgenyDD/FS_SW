#include "platform.h"
#include "fw_header.h"
#include "ret_mem.h"
#include <string.h>

#if FW_TYPE == FW_LDR || FW_TYPE == FW_APP
#include "rfm12b.h"
#endif

extern void usb_disconnect(void);

typedef void (*pFunction)(void);
uint32_t JumpAddress;
pFunction Jump_To_Application;

typedef struct
{
	uint32_t start;
	uint32_t len;
	uint32_t num;
	bool erased;
} sector_t;

#define CFG_SECT 4

sector_t flash_desc[] = {
	{0x08000000, 16 * 1024, FLASH_Sector_0, false},
	{0x08004000, 16 * 1024, FLASH_Sector_1, false},
	{0x08008000, 16 * 1024, FLASH_Sector_2, false},
	{0x0800C000, 16 * 1024, FLASH_Sector_3, false},
	{0x08010000, 64 * 1024, FLASH_Sector_4, false},
	{0x08020000, 128 * 1024, FLASH_Sector_5, false},
	{0x08040000, 128 * 1024, FLASH_Sector_6, false},
	{0x08060000, 128 * 1024, FLASH_Sector_7, false},
};

static int find_sector(uint32_t addr)
{
	for(unsigned int i = 0; i < sizeof(flash_desc) / sizeof(flash_desc[0]); i++)
	{
		if((addr >= flash_desc[i].start) && (addr < (flash_desc[i].start + flash_desc[i].len)))
		{
			return (int)i;
		}
	}
	return -1;
}

void platform_flash_erase_flag_reset(void)
{
	for(unsigned int i = 0; i < sizeof(flash_desc) / sizeof(flash_desc[0]); i++)
	{
		flash_desc[i].erased = false;
	}
}

void platform_flash_erase_flag_reset_sect_cfg(void)
{
	flash_desc[CFG_SECT].erased = false;
}

void platform_flash_unlock(void) { FLASH_Unlock(); }

void platform_flash_lock(void) { FLASH_Lock(); }

int platform_flash_write(uint32_t dest, const uint8_t *src, uint32_t sz)
{
	if(sz == 0) return 0;

	platform_flash_unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
	int sts;
	for(uint32_t i = 0; i < sz; i++)
	{
		int sect = find_sector(dest + i);
		if(sect < 0)
		{
			platform_flash_lock();
			return 1;
		}

		if(!flash_desc[sect].erased)
		{
			flash_desc[sect].erased = true;
			sts = FLASH_EraseSector(flash_desc[sect].num, VoltageRange_3);
			if(sts != FLASH_COMPLETE)
			{
				platform_flash_lock();
				return 2;
			}
		}

		sts = FLASH_ProgramByte(dest + i, *(src + i));
		if(sts != FLASH_COMPLETE)
		{
			platform_flash_lock();
			return 4;
		}
		if(*(__IO uint8_t *)(dest + i) != *(src + i))
		{
			platform_flash_lock();
			return 5;
		}
	}
	platform_flash_lock();
	return 0;
}

int platform_flash_read(uint32_t addr, uint8_t *src, uint32_t sz)
{
	if(addr < FLASH_START || addr + sz >= FLASH_FINISH) return 1;
	memcpy(src, (void *)addr, sz);
	return 0;
}

// ========================================================================

void platform_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; // BTN SNS
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// === === === === ===

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// === === === === ===

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	{
		USART_DeInit(USART3);
		USART_InitTypeDef USART_InitStruct = {0};
		USART_InitStruct.USART_BaudRate = 115200;
		USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_InitStruct.USART_WordLength = USART_WordLength_8b;
		USART_InitStruct.USART_StopBits = USART_StopBits_1;
		USART_InitStruct.USART_Parity = USART_Parity_No;
		USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_Init(USART3, &USART_InitStruct);
		USART_Cmd(USART3, ENABLE);
	}

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
	TIM_ICInitTypeDef TIM_ICInitStructure = {0};
	TIM_OCInitTypeDef TIM_OCInitStructure = {0};

	// TIM1
	{
		TIM_TimeBaseStructure.TIM_Prescaler = 11;
		TIM_TimeBaseStructure.TIM_Period = 39999;
		TIM_TimeBaseStructure.TIM_ClockDivision = 0;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
		TIM_OCInitStructure.TIM_Pulse = 0;
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
		TIM_OC1Init(TIM1, &TIM_OCInitStructure);
		TIM_OC2Init(TIM1, &TIM_OCInitStructure);
		TIM_OC3Init(TIM1, &TIM_OCInitStructure);
		TIM_OC4Init(TIM1, &TIM_OCInitStructure);
		TIM_ARRPreloadConfig(TIM1, ENABLE);
		TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
		TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
		TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
		TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
		TIM_Cmd(TIM1, ENABLE);
		TIM_CtrlPWMOutputs(TIM1, ENABLE);
	}

	// TIM2
	{
		TIM_TimeBaseStructure.TIM_Prescaler = 14;
		TIM_TimeBaseStructure.TIM_Period = 99;
		TIM_TimeBaseStructure.TIM_ClockDivision = 0;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
		TIM_OCInitStructure.TIM_Pulse = 0;
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
		TIM_OC1Init(TIM2, &TIM_OCInitStructure);
		TIM_OC3Init(TIM2, &TIM_OCInitStructure);
		TIM_OC4Init(TIM2, &TIM_OCInitStructure);
		TIM_ARRPreloadConfig(TIM2, ENABLE);
		TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
		TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
		TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);
		TIM_Cmd(TIM2, ENABLE);
	}

	// TIM4
	{
		TIM_TimeBaseStructure.TIM_Prescaler = 11;
		TIM_TimeBaseStructure.TIM_Period = 39999;
		TIM_TimeBaseStructure.TIM_ClockDivision = 0;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
		TIM_OCInitStructure.TIM_Pulse = 0;
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
		TIM_OC1Init(TIM4, &TIM_OCInitStructure);
		TIM_OC2Init(TIM4, &TIM_OCInitStructure);
		TIM_OC3Init(TIM4, &TIM_OCInitStructure);
		TIM_ARRPreloadConfig(TIM4, ENABLE);
		TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
		TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
		TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
		TIM_Cmd(TIM4, ENABLE);
	}

	// TIM8 - servo pwm capture
	{
		TIM_TimeBaseStructure.TIM_Period = 11;
		TIM_TimeBaseStructure.TIM_Prescaler = 39999;
		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

		TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
		TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
		TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
		TIM_ICInitStructure.TIM_ICFilter = 0;
		TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
		TIM_ICInit(TIM8, &TIM_ICInitStructure);
		TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
		TIM_ICInit(TIM8, &TIM_ICInitStructure);
		TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
		TIM_ICInit(TIM8, &TIM_ICInitStructure);
		TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
		TIM_ICInit(TIM8, &TIM_ICInitStructure);

		TIM_Cmd(TIM8, ENABLE);
		TIM_CCxCmd(TIM8, TIM_Channel_1, TIM_CCx_Enable);
		TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Enable);
		TIM_CCxCmd(TIM8, TIM_Channel_3, TIM_CCx_Enable);
		TIM_CCxCmd(TIM8, TIM_Channel_4, TIM_CCx_Enable);
	}

	// TIM12
	{
		TIM_TimeBaseStructure.TIM_Prescaler = 14;
		TIM_TimeBaseStructure.TIM_Period = 99;
		TIM_TimeBaseStructure.TIM_ClockDivision = 0;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);

		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
		TIM_OCInitStructure.TIM_Pulse = 0;
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
		TIM_OC1Init(TIM12, &TIM_OCInitStructure);
		TIM_OC2Init(TIM12, &TIM_OCInitStructure);
		TIM_ARRPreloadConfig(TIM12, ENABLE);
		TIM_OC1PreloadConfig(TIM12, TIM_OCPreload_Enable);
		TIM_OC2PreloadConfig(TIM12, TIM_OCPreload_Enable);
		TIM_Cmd(TIM12, ENABLE);
	}

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_TIM1);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_TIM4);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM8);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM8);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM8);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM8);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM12);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

	GPIOB->BSRRL = 1 << 12; // enable power FET
}

void platform_deinit(void)
{
	__disable_irq();
	SysTick->CTRL = 0;
	for(uint32_t i = 0; i < sizeof(NVIC->ICPR) / sizeof(NVIC->ICPR[0]); i++)
	{
		NVIC->ICPR[i] = 0xfffffffflu;
	}
	__enable_irq();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winline"
__attribute__((noreturn)) void platform_reset(void)
{
#if FW_TYPE == FW_LDR
	ret_mem_set_bl_stuck(false);
#endif
	platform_deinit();
	NVIC_SystemReset();
}

__attribute__((noreturn)) void platform_reset_jump_ldr_app(void)
{
#if FW_TYPE == FW_LDR
	ret_mem_set_bl_stuck(false);
#endif
	platform_deinit();
	fw_header_check_all();
#if FW_TYPE == FW_APP
	if(g_fw_info[FW_LDR].locked == false) platform_run_address((uint32_t)&__ldr_start);
#endif
#if FW_TYPE == FW_LDR
	if(g_fw_info[FW_APP].locked == false) platform_run_address((uint32_t)&__app_start);
#endif
	NVIC_SystemReset();
}
#pragma GCC diagnostic pop

__attribute__((optimize("-O0"))) __attribute__((always_inline)) static __inline void boot_jump(uint32_t address)
{
	JumpAddress = *(__IO uint32_t *)(address + 4);
	Jump_To_Application = (pFunction)JumpAddress;
	__set_MSP(*(__IO uint32_t *)address);
	Jump_To_Application();
}

__attribute__((optimize("-O0"))) void platform_run_address(uint32_t address)
{
	platform_deinit();
	SCB->VTOR = address;
	boot_jump(address);
}

void platform_get_uid(uint32_t *id)
{
	memcpy(id, (void *)UNIQUE_ID, 3 * sizeof(uint32_t));
}

void platform_watchdog_init(void)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(IWDG_Prescaler_256);
	IWDG_SetReload(10000 /*time_ms*/ / 8);
	IWDG_Enable();
	IWDG_ReloadCounter();
}

static const char *get_rst_src(void)
{
	if(RCC_GetFlagStatus(RCC_FLAG_LPWRRST)) return "low power";
	if(RCC_GetFlagStatus(RCC_FLAG_WWDGRST)) return "window watchdog";
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST)) return "independent watchdog";
	if(RCC_GetFlagStatus(RCC_FLAG_SFTRST)) return "software";
	if(RCC_GetFlagStatus(RCC_FLAG_PORRST)) return "power on power down";
	if(RCC_GetFlagStatus(RCC_FLAG_PINRST)) return "reset pin";
	if(RCC_GetFlagStatus(RCC_FLAG_BORRST)) return "brownout";
	return "unknown";
}

const char *platform_reset_cause_get(void)
{
	const char *src = get_rst_src();
	RCC_ClearFlag();
	return src;
}

void _lseek_r(void) {}
void _close_r(void) {}
void _read_r(void) {}
void _write_r(void) {}

void pwr_off(void)
{
	GPIOB->BSRRH = 1 << 12; // disable power FET
}

bool fan_set(uint8_t fan, uint32_t value)
{
	if(value > 99) value = 99;
	__IO uint32_t *ptr[] = {
		&TIM12->CCR1,
		&TIM12->CCR2};
	if(fan > 1) return true;
	*ptr[fan] = value;
	return false;
}