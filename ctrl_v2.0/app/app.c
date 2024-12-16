#include "adc.h"
#include "air_protocol.h"
#include "config_system.h"
#include "console.h"
#include "debounce.h"
#include "dfu_sub_flash.h"
#include "fw_header.h"
#include "hb_tracker.h"
#include "led_drv.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "rfm75.h"
#include "usb_hw.h"
#include "usbd_core_cdc.h"
#include <string.h>

// #define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

#define NODE_HEAD 0

bool g_stay_in_boot = false;
uint32_t g_uid[3];
volatile uint32_t system_time_ms = 0;

uint8_t rf_channel = 42;
config_entry_t g_device_config[] = {{"rf_chnl", sizeof(rf_channel), 0, &rf_channel}};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

static int32_t prev_systick = 0;
static debounce_t btn[4][3];
static uint32_t to_without_press = 0;
static bool btn_pwr_off_long_press = false;

static airproto_head_sts_t head_last_rpt = {0};

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
	ret_mem_set_load_src(LOAD_SRC_APP); // let preboot know it was booted from app

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	hb_tracker_init(NODE_HEAD, 1000);

	for(uint32_t i = 0; i < 4; i++)
		for(uint32_t k = 0; k < 3; k++)
			debounce_init(&btn[i][k], 2000);

	usb_init();
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
		usb_poll(diff_ms);
		dfu_sub_poll(diff_ms);
		BUTTONS_POLL();

		led_drv_set_led(LED_0, btn_pwr_off_long_press ? LED_MODE_ON : LED_MODE_SFLASH_2_5HZ);
		led_drv_set_led(LED_4, usb_is_configured() ? LED_MODE_ON : LED_MODE_OFF);

		// self-off control
		to_without_press += diff_ms;
		for(uint32_t i = 0; i < 4; i++)
			for(uint32_t k = 0; k < 3; k++)
				if(btn[i][k].state)
				{
					usbd_cdc_unlock();
					to_without_press = 0;
				}
		if(hb_tracker_is_timeout(NODE_HEAD) == false) to_without_press = 0;
		////// if(pwr_is_charging()) to_without_press = 0; // don't work ok
		if(usb_is_configured()) to_without_press = 0;

		if(usb_is_configured() == false && (to_without_press > 15000 || adc_val.v_bat < 3.2f)) pwr_off();

		// off button
		{
			if(btn[0][0].state == BTN_PRESS_LONG_SHOT) btn_pwr_off_long_press = true;
			if(btn[0][0].state == BTN_UNPRESS_SHOT && btn_pwr_off_long_press) pwr_off();
		}

		static uint32_t poll_cnt = 0;
		poll_cnt += diff_ms;
		if(poll_cnt >= 151)
		{
			poll_cnt = 0;
			if(rfm75_is_delayed() == false) rfm75_tx(0, (uint8_t[]){AIRPROTO_CMD_POLL}, 1);
		}

		led_drv_set_led(LED_7, hb_tracker_is_timeout(NODE_HEAD) ? LED_MODE_FLASH_10HZ : LED_MODE_STROB_1HZ);

		static int ear_move_step = -1;
		static uint32_t ear_move_delay = 0;

		if(ear_move_step != -1 && ear_move_delay < system_time_ms)
		{
			static uint8_t data_ear_smooth_servo[] = {AIRPROTO_CMD_SERVO_SMOOTH, 0, 0,
													  2 /*0*/, 0, 0,
													  3 /*1*/, 0, 0};
			uint16_t time = 300;
			memcpy(&data_ear_smooth_servo[1], &time, 2);
			uint16_t pos[2][2] = {
				{750, 300},
				{350, 750},
			};

			for(uint32_t i = 0; i < 2; i++)
				memcpy(&data_ear_smooth_servo[3 + (3 * i + 1)], &pos[ear_move_step % 2][i], 2);
			rfm75_tx_force(NODE_HEAD, data_ear_smooth_servo, sizeof(data_ear_smooth_servo));

			if(++ear_move_step >= 4) ear_move_step = -1;
			ear_move_delay = system_time_ms + 400;
		}

#define CYCLIC_OP(num, op)                \
	{                                     \
		for(uint32_t i = 0; i < num; i++) \
			op;                           \
	}

		// button handler
		{
			if(btn[0][0].state == BTN_PRESS_SHOT)
			{
				// disable all servos
				uint8_t data[] = {AIRPROTO_CMD_SERVO_RAW, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
				rfm75_tx_force(NODE_HEAD, data, sizeof(data));
			}

			if(btn[1][0].state == BTN_PRESS_SHOT)
			{
				ear_move_step = 0;
				ear_move_delay = 0;
			}

			if(btn[0][1].state == BTN_PRESS_SHOT)
			{
				static const uint8_t fan_lo[] = {0, 60, 80, 95};
				static const uint8_t fan_hi[] = {0, 0, 70, 95};
				static uint32_t mode = 0;
				mode++;
				if(mode >= sizeof(fan_lo) / sizeof(fan_lo[0])) mode = 0;
				uint8_t data[] = {AIRPROTO_CMD_FAN, 0, 0};
				data[1] = fan_hi[mode];
				data[2] = fan_lo[mode];
				rfm75_tx_force(NODE_HEAD, data, sizeof(data));
			}

			if(btn[1][1].state == BTN_PRESS_SHOT)
			{
				static const uint8_t leds[3][4] = {{0, 120, 0, 0},
												   {0, 0, 120, 0},
												   {0, 0, 0, 120}};
				static uint32_t mode = 0;
				mode++;
				if(mode >= 4) mode = 0;
				uint8_t data[] = {AIRPROTO_CMD_LIGHT, 0, 0, 0};
				data[1] = leds[0][mode];
				data[2] = leds[1][mode];
				data[3] = leds[2][mode];
				rfm75_tx_force(NODE_HEAD, data, sizeof(data));
			}

			if(btn[2][1].state == BTN_PRESS_SHOT)
			{
				static const uint16_t servo_tongue[] = {320, /*470,*/ 620};
				static uint32_t mode = 0;
				mode++;
				if(mode >= sizeof(servo_tongue) / sizeof(servo_tongue[0])) mode = 0;
				uint8_t data[] = {AIRPROTO_CMD_SERVO_RAW, 6, 0, 0};
				memcpy(&data[2], &servo_tongue[mode], 2);
				rfm75_tx_force(NODE_HEAD, data, sizeof(data));
			}

			static uint8_t data_smooth_servo[] = {AIRPROTO_CMD_SERVO_SMOOTH, 0, 0,
												  0 /*0*/, 0, 0,
												  1 /*1*/, 0, 0,
												  4 /*4*/, 0, 0,
												  5 /*5*/, 0, 0};
			if(btn[0][2].state == BTN_PRESS_SHOT)
			{
				uint16_t time = 800;
				memcpy(&data_smooth_servo[1], &time, 2);
				uint16_t pos[4] = {500, 560, 370, 330};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				rfm75_tx_force(NODE_HEAD, data_smooth_servo, sizeof(data_smooth_servo));
			}

			if(btn[1][2].state == BTN_PRESS_SHOT)
			{
				uint16_t time = 800;
				memcpy(&data_smooth_servo[1], &time, 2);
				uint16_t pos[4] = {340, 380, 600, 500};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				rfm75_tx_force(NODE_HEAD, data_smooth_servo, sizeof(data_smooth_servo));
			}

			if(btn[2][2].state == BTN_PRESS_SHOT)
			{
				uint16_t pos[4] = {500, 420, 550, 330};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				rfm75_tx_force(NODE_HEAD, data_smooth_servo, sizeof(data_smooth_servo));
			}

			if(btn[3][2].state == BTN_PRESS_SHOT)
			{
				// uint16_t pos[4] = {440,330,640,400}; // alt
				uint16_t pos[4] = {500, 330, 620, 330};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				rfm75_tx_force(NODE_HEAD, data_smooth_servo, sizeof(data_smooth_servo));
			}
		}
	}
}

void usbd_cdc_rx(const uint8_t *data, uint32_t size)
{
	if(size > 1 && data[0] == '*')
		rfm75_tx_force(NODE_HEAD, data + 1, size - 1);
	else
		console_cb(data, size);
}

void rfm75_tx_fail(uint8_t lost, uint8_t rt) {}
void rfm75_tx_done(void) {}

void rfm75_process_data(uint8_t dev_idx, const uint8_t *data, uint8_t data_len)
{
	hb_tracker_update(dev_idx);

	switch(data[0])
	{
	case AIRPROTO_CMD_POLL:
	default: break;

	case AIRPROTO_CMD_REBOOT: platform_reset_jump_ldr_app(); break;
	case AIRPROTO_CMD_OFF: pwr_off(); break;
	case AIRPROTO_CMD_DEBUG:
		usbd_cdc_push_data((const uint8_t *)"*", 1);
		usbd_cdc_push_data(data + 1, data_len - 1);
		break;

	case AIRPROTO_CMD_FLASH_STS: rf_fw_sts_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_TYPE: rf_fw_type_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_WRITE: rf_flash_write_cb(dev_idx, data, data_len); break;
	case AIRPROTO_CMD_FLASH_READ: rf_flash_read_cb(dev_idx, data, data_len); break;

	case AIRPROTO_CMD_STS:
		if(dev_idx == NODE_HEAD && data_len == sizeof(airproto_head_sts_t))
		{
			memcpy(&head_last_rpt, data, data_len);
			console_print("=H: %.3fV | %d %d %d | %d %d %d %d %d %d %d\n",
						  0.001f * (float)head_last_rpt.vbat_mv, head_last_rpt.t_mcu, head_last_rpt.t_ntc[0], head_last_rpt.t_ntc[1],
						  head_last_rpt.s_pos[0], head_last_rpt.s_pos[1], head_last_rpt.s_pos[2], head_last_rpt.s_pos[3],
						  head_last_rpt.s_pos[4], head_last_rpt.s_pos[5], head_last_rpt.s_pos[6]);
		}
		break;
	}
}