#include "adc.h"
#include "config_system.h"
#include "console.h"
#include "crc.h"
#include "debounce.h"
#include "dfu_sub_flash.h"
#include "fw_header.h"
#include "led_drv.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "rfm12b.h"
#include "usb_hw.h"
#include "usbd_core_cdc.h"

#define BOOT_DELAY 500

#define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

bool g_stay_in_boot = false;
volatile uint32_t system_time_ms = 0;
uint32_t g_uid[3];

uint8_t rf_channel = 42;
config_entry_t g_device_config[] = {{"rf_chnl", sizeof(rf_channel), 0, &rf_channel}};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

static volatile uint32_t boot_delay = BOOT_DELAY;
static int32_t prev_systick = 0;
static debounce_t btn[4][3];
static uint32_t to_without_press = 0;
static bool btn_pwr_off_long_press = false;

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
	// platform_watchdog_init();

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

	// air_protocol_init();
	rfm12b_init(0xAA);

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
			platform_reset_jump_ldr_app();
		}

		adc_track();
		led_drv_poll(diff_ms);
		// air_protocol_poll();
		// hb_tracker_poll(diff_ms);
		usb_poll(diff_ms);
		dfu_sub_poll(diff_ms);
		rfm12b_poll(diff_ms);
		BUTTONS_POLL();

		boot_delay = boot_delay >= diff_ms ? boot_delay - diff_ms : 0;

		if(system_time_ms > 1000)
		{
			static uint32_t d = 0;
			d += diff_ms;
			if(d >= 500)
			{
				d = 0;
				rfm12b_tx(0, "ABCDABCDABCDABCDABCDABCDABCDABCD", 32);
			}
		}

		// if(hb_tracker_is_timeout(NODE_HEAD) == false) to_without_press = 0;

		// if(usb_is_configured() == false &&
		//    system_time_ms > 2000 &&
		//    (to_without_press > 15000 || adc_val.v_bat < 3.2f)) pwr_off();

		// // off button
		// {
		// 	if(btn[0][0].state == BTN_PRESS_LONG_SHOT) btn_pwr_off_long_press = true;
		// 	if(btn[0][0].state == BTN_UNPRESS_SHOT && btn_pwr_off_long_press) pwr_off();
		// }

		// static uint32_t poll_cnt = 0;
		// poll_cnt += diff_ms;
		// if(poll_cnt >= 151)
		// {
		// 	poll_cnt = 0;
		// 	if(rfm75_is_delayed() == false) rfm75_tx(0, (uint8_t[]){AIRPROTO_CMD_POLL}, 1);
		// }
	}
}

void rfm12b_process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
}

void usbd_cdc_rx(const uint8_t *data, uint32_t size)
{
	// if(size > 1 && data[0] == '*')
	// rfm75_tx_force(NODE_HEAD, data + 1, size - 1);
	// else
	// console_cb(data, size);
}