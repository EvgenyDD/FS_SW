#include "adc.h"
#include "air_protocol.h"
#include "config_system.h"
#include "console.h"
#include "debounce.h"
#include "debug.h"
#include "fw_header.h"
#include "hb_tracker.h"
#include "led.h"
#include "led_drv.h"
#include "prof.h"
#include "ret_mem.h"
#include "rfm75.h"
#include "servo.h"
#include <string.h>

#define MASTER_TO_MS_SHUTDOWN 10000

#define SYSTICK_IN_US (48000000 / 1000000)
#define SYSTICK_IN_MS (48000000 / 1000)

#define NODE_CTRL 0

bool g_stay_in_boot = false;
uint32_t g_uid[3];
volatile uint32_t system_time_ms = 0;

uint8_t rf_channel = 42;
config_entry_t g_device_config[] = {{"rf_chnl", sizeof(rf_channel), 0, &rf_channel}};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

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

static void power_off_animation(uint32_t diff_ms)
{
	static bool done = false;
	static uint32_t cnt = 0;
	if(!done)
	{
		if(cnt == 0)
		{
			led_set(0, 200);
			led_set(1, 0);
			led_set(2, 0);
		}
		if((cnt % 1000) > 500)
		{
			led_set(0, 0);
		}
		else
		{
			led_set(0, 200);
		}
		cnt += diff_ms;
		if(cnt > 3000) done = true;
	}
	if(done) pwr_off();
}

void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	platform_init();
	adc_init();
	servo_init();

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_APP); // let preboot know it was booted from app

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

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

		adc_track();
		led_drv_poll(diff_ms);
		rfm75_poll(diff_ms);
		hb_tracker_poll(diff_ms);
		debounce_update(&btn_pwr, GPIOA->IDR & (1 << 12), diff_ms);

		led_drv_set_led(LED_0, hb_tracker_is_timeout(NODE_CTRL) ? LED_MODE_SFLASH_2_5HZ : LED_MODE_STROB_1HZ);

		if(hb_tracker_get_timeout(NODE_CTRL) > MASTER_TO_MS_SHUTDOWN ||
		   btn_pwr.state == BTN_UNPRESS_SHOT ||
		   adc_val.v_bat < 3.1f * 2.f)
		{
			power_off_animation(diff_ms);
		}

		servo_poll(diff_ms);

		static uint32_t ctr_sts_head = 0;
		if(ctr_sts_head < system_time_ms)
		{
			ctr_sts_head = system_time_ms + 330;
			airproto_head_sts_t rpt = {
				.hdr = AIRPROTO_CMD_STS,
				.vbat_mv = (uint16_t)(adc_val.v_bat * 1000.0f),
				.t_mcu = (int8_t)adc_val.t_mcu,
				.t_ntc[0] = adc_val.t_ntc[0],
				.t_ntc[1] = adc_val.t_ntc[1],
			};
			for(uint32_t i = 0; i < 7; i++)
			{
				rpt.s_pos[i] = adc_val.s_pos[i];
			}
			rfm75_tx_force(NODE_CTRL, (uint8_t *)&rpt, sizeof(rpt));
		}
	}
}

void rfm75_tx_done(void) {}
void rfm75_tx_fail(uint8_t lost, uint8_t rt) {}

#define TIMEOUT_DEFAULT 250
#define TIMEOUT_WRITE 4000

#if FW_TYPE == FW_LDR
#define FW_TARGET FW_APP
#define ADDR_ORIGIN ((uint32_t) & __app_start)
#define ADDR_END ((uint32_t) & __app_end)
#elif FW_TYPE == FW_APP
#define FW_TARGET FW_LDR
#define ADDR_ORIGIN ((uint32_t) & __ldr_start)
#define ADDR_END ((uint32_t) & __ldr_end)
#endif

void rfm75_process_data(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	hb_tracker_update(dev_idx);

	switch(data[0])
	{
	case AIRPROTO_CMD_POLL:
	default: break;

	case AIRPROTO_CMD_REBOOT: platform_reset_jump_ldr_app(); break;
	case AIRPROTO_CMD_OFF: pwr_off(); break;
	case AIRPROTO_CMD_DEBUG: console_cb((const char *)(data + 1), data_len - 1); break;

	case AIRPROTO_CMD_FLASH_STS: air_flash_sts_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_TYPE: air_flash_type_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_WRITE: air_flash_write_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_READ: air_flash_read_cb(dev_idx, data, data_len); break;

	case AIRPROTO_CMD_LIGHT:
		if(data_len == 4)
		{
			led_set_gamma(0, data[1]);
			led_set_gamma(1, data[2]);
			led_set_gamma(2, data[3]);
		}
		break;

	case AIRPROTO_CMD_FAN:
		if(data_len == 3)
		{
			fan_set(0, data[1]);
			fan_set(1, data[2]);
		}
		break;

	case AIRPROTO_CMD_SERVO_RAW:
		if(data_len == 1 + SERVO_COUNT * 2) // all servos
		{
			for(uint32_t i = 0; i < SERVO_COUNT; i++)
			{
				uint16_t signal;
				memcpy(&signal, &data[1U + 2U * i], 2);
				servo_set(i, signal);
			}
		}
		else if(data_len == 1 + 1 /* selector */ + 2 /* data */) // single servo
		{
			if(data[1] < SERVO_COUNT)
			{
				uint16_t signal;
				memcpy(&signal, &data[2], 2);
				servo_set(data[1], signal);
			}
		}
		break;

	case AIRPROTO_CMD_SERVO_SMOOTH:
		if(data_len >= 1 + 2 /* time */ + 1 /* selector */ + 2 /* data */)
		{
			if(((data_len - 3) % 3) == 0)
			{
				uint16_t time;
				memcpy(&time, &data[1], 2);
				for(uint32_t i = 3; i < data_len;)
				{
					uint16_t pos;
					memcpy(&pos, &data[i + 1], 2);
					servo_set_smooth_and_off(data[i], pos, time);
					i += 3;
				}
			}
		}
		break;
	}
}