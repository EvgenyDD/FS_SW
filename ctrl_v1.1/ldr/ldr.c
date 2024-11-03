#include "adc.h"
#include "air_protocol.h"
#include "config_system.h"
#include "crc.h"
#include "debounce.h"
#include "fw_header.h"
#include "led_drv.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "usbd_proto_core.h"

#define BOOT_DELAY 500

#define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

bool g_stay_in_boot = false;

volatile uint32_t system_time_ms = 0;

static volatile int boot_delay = BOOT_DELAY;
static int32_t prev_systick = 0;

config_entry_t g_device_config[] = {};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

uint32_t g_uid[3];

static debounce_t btn[4][3];

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit)
			return;
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

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	if(ret_mem_is_bl_stuck()) g_stay_in_boot = true;

	for(uint32_t i = 0; i < 4; i++)
		for(uint32_t k = 0; k < 3; k++)
			debounce_init(&btn[i][k], 2000);

	usb_init();

	air_protocol_init();

	for(;;)
	{
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		// uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		platform_watchdog_reset();

		if(led_startup_sample())
		{
			static int ii = 0, cnt = 0;
			cnt += diff_ms;
			if(cnt >= 600)
			{
				cnt = 0;
				if(++ii >= 8) ii = 0;
				for(uint32_t i = 0; i < 8; i++)
					led_drv_set_led(i, LED_MODE_OFF);
				led_drv_set_led(ii, LED_MODE_FLASH_10HZ);
			}
		}

		if(!boot_delay &&
		   !g_stay_in_boot &&
		   g_fw_info[FW_APP].locked == false)
		{
			platform_reset();
		}

		adc_track();

		air_protocol_poll();

		led_drv_poll(diff_ms);

		usb_poll(diff_ms);

		system_time_ms += diff_ms;

		boot_delay = boot_delay >= diff_ms ? boot_delay - diff_ms : 0;

		debounce_update(&btn[0][0], !(GPIOC->IDR & (1 << 14)), diff_ms);
		debounce_update(&btn[0][1], !(GPIOC->IDR & (1 << 11)), diff_ms);
		debounce_update(&btn[0][2], !(GPIOC->IDR & (1 << 8)), diff_ms);

		debounce_update(&btn[1][0], !(GPIOC->IDR & (1 << 15)), diff_ms);
		debounce_update(&btn[1][1], !(GPIOA->IDR & (1 << 15)), diff_ms);
		debounce_update(&btn[1][2], !(GPIOC->IDR & (1 << 7)), diff_ms);

		debounce_update(&btn[2][0], !(GPIOA->IDR & (1 << 4)), diff_ms);
		debounce_update(&btn[2][1], !(GPIOC->IDR & (1 << 10)), diff_ms);
		debounce_update(&btn[2][2], !(GPIOB->IDR & (1 << 15)), diff_ms);

		debounce_update(&btn[3][0], !(GPIOA->IDR & (1 << 5)), diff_ms);
		debounce_update(&btn[3][1], !(GPIOA->IDR & (1 << 3)), diff_ms);
		debounce_update(&btn[3][2], !(GPIOB->IDR & (1 << 12)), diff_ms);
	}
}

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
}