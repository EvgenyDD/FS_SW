#include "adc.h"
#include "air_protocol.h"
#include "config_system.h"
#include "crc.h"
#include "debounce.h"
#include "debug.h"
#include "fw_header.h"
#include "hb_tracker.h"
#include "led_drv.h"
#include "prof.h"
#include "ret_mem.h"
#include "rfm75.h"

#define MASTER_TO_MS_SHUTDOWN 10000
#define BOOT_DELAY 500

#define SYSTICK_IN_US (48000000 / 1000000)
#define SYSTICK_IN_MS (48000000 / 1000)

#define NODE_CTRL 0

bool g_stay_in_boot = false;
uint32_t g_uid[3];
volatile uint32_t system_time_ms = 0;

uint8_t rf_channel = 42;
config_entry_t g_device_config[] = {{"rf_chnl", sizeof(rf_channel), 0, &rf_channel}};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

static volatile uint32_t boot_delay = BOOT_DELAY;
static int32_t prev_systick = 0;
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

	debounce_init(&btn_pwr, 2000);
	btn_pwr.int_state_prev = btn_pwr.int_state = btn_pwr.state = BTN_PRESS;

	hb_tracker_init(NODE_CTRL, 250);

	rfm75_init();

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

		boot_delay = boot_delay >= diff_ms ? boot_delay - diff_ms : 0;

		adc_track();
		led_drv_poll(diff_ms);
		rfm75_poll(diff_ms);
		hb_tracker_poll(diff_ms);
		debounce_update(&btn_pwr, GPIOA->IDR & (1 << 12), diff_ms);

		led_drv_set_led(LED_0, hb_tracker_is_timeout(NODE_CTRL) ? LED_MODE_FLASH_10HZ : LED_MODE_STROB_5HZ);

		if(hb_tracker_get_timeout(NODE_CTRL) > MASTER_TO_MS_SHUTDOWN ||
		   btn_pwr.state == BTN_UNPRESS_SHOT ||
		   adc_val.v_bat < 3.1f * 2.f)
		{
			pwr_off();
		}
	}
}

void rfm75_tx_done(void) {}
void rfm75_tx_fail(uint8_t lost, uint8_t rt) {}

static void parse_debug(const char *s, uint32_t len)
{
	debug_rf("Can't parse debug in BOOTLOADER!\n");
}

void rfm75_process_data(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	hb_tracker_update(dev_idx);

	switch(data[0])
	{
	case AIRPROTO_CMD_POLL:
	default: break;

	case AIRPROTO_CMD_REBOOT: platform_reset_jump_ldr_app(); break;
	case AIRPROTO_CMD_OFF: pwr_off(); break;
	case AIRPROTO_CMD_DEBUG: parse_debug((const char *)(data + 1), data_len - 1); break;

	case AIRPROTO_CMD_FLASH_STS: air_flash_sts_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_TYPE: air_flash_type_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_WRITE: air_flash_write_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_READ: air_flash_read_cb(dev_idx, data, data_len); break;
	}
}
