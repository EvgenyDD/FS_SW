#include "adc.h"
#include "config_system.h"
#include "crc.h"
#include "debounce.h"
#include "fw_header.h"
#include "led_drv.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "rfm12b.h"
#include <string.h>

#define BOOT_DELAY 500

#define SYSTICK_IN_US (48000000 / 1000000)
#define SYSTICK_IN_MS (48000000 / 1000)

bool g_stay_in_boot = false;

volatile uint32_t system_time_ms = 0;

static volatile uint32_t boot_delay = BOOT_DELAY;
static int32_t prev_systick = 0;

const uint8_t iterator_ctrl = 0;

uint32_t g_uid[3];

static debounce_t btn_pwr;

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit) return;
	}
}

void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	platform_init();
	adc_init();

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_BOOTLOADER); // let preboot know it was booted from bootloader

	if(ret_mem_is_bl_stuck()) g_stay_in_boot = true;

	debounce_init(&btn_pwr, 2000);
	btn_pwr.int_state_prev = btn_pwr.int_state = btn_pwr.state = BTN_PRESS;

	rfm12b_init(0xAA);

	led_drv_set_led(LED_0, LED_MODE_FLASH_10HZ);

	for(;;)
	{
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t /*remain_systick_us_prev = 0,*/ remain_systick_ms_prev = 0;
		// uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		// remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		system_time_ms += diff_ms;
		platform_watchdog_reset();

		if(!boot_delay &&
		   !g_stay_in_boot &&
		   g_fw_info[FW_APP].locked == false)
		{
			platform_reset();
		}

		adc_track();
		led_drv_poll(diff_ms);
		rfm12b_poll(diff_ms);
		debounce_update(&btn_pwr, GPIOA->IDR & (1 << 12), diff_ms);

		boot_delay = boot_delay >= diff_ms ? boot_delay - diff_ms : 0;

		if(system_time_ms > 1000)
		{
			static uint32_t d = 0;
			d += diff_ms;
			if(d >= 500)
			{
				d = 0;
				// rfm12b_tx(0, "ABCDABCDABCDABCDABCDABCDABCDABCD", 32);
			}
		}
	}
}

void rfm12b_process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
	switch(data[0])
	{
	default: break;
	}
}
