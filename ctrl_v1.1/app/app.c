#include "adc.h"
#include "air_protocol.h"
#include "config_system.h"
#include "crc.h"
#include "debounce.h"
#include "fw_header.h"
#include "hb_tracker.h"
#include "led_drv.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "rfm12b.h"
#include "usbd_proto_core.h"
#include <stdio.h>
#include <string.h>

int gsts = -10;

#define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

bool g_stay_in_boot = false;

uint32_t g_uid[3];

volatile uint32_t system_time_ms = 0;
static int32_t prev_systick_poll = 0;

config_entry_t g_device_config[] = {};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

static debounce_t btn[4][3];
static uint32_t to_without_press = 0;

const uint8_t iterator_head = 0;
const uint8_t iterator_tail = 1;

hbt_node_t *hbt_head;

bool to_from_head = true;

bool g_charge_en = true;

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
		if(start >= time_limit)
			return;
	}
}

void poll_main(void)
{
	uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick_poll);
	static uint32_t remain_systick_ms_prev = 0;
	uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
	remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

	system_time_ms += diff_ms;

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

	adc_track();

	led_drv_poll(diff_ms);

	usb_poll(diff_ms);

	led_drv_set_led(LED_0, btn_pwr_off_long_press ? LED_MODE_ON : LED_MODE_SFLASH_2_5HZ);
	led_drv_set_led(LED_4, usb_is_configured() ? LED_MODE_ON : LED_MODE_OFF);

	// self-off control
	to_without_press += diff_ms;
	for(uint32_t i = 0; i < 4; i++)
		for(uint32_t k = 0; k < 3; k++)
			if(btn[i][k].state) to_without_press = 0;
	if(hb_tracker_is_timeout(hbt_head) == false) to_without_press = 0;
	////// if(pwr_is_charging()) to_without_press = 0; // don't work ok
	if(usb_is_configured()) to_without_press = 0;

	if(usb_is_configured() == false)
		if(to_without_press > 15000 || adc_val.v_bat < 3.2f) pwr_sleep();

	// off button
	{
		if(btn[0][0].state == BTN_PRESS_LONG_SHOT) btn_pwr_off_long_press = true;
		if(btn[0][0].state == BTN_UNPRESS_SHOT && btn_pwr_off_long_press) pwr_sleep();
	}
}

void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	platform_init();

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_7; // DCDC_FAULT, SNS_KEY
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	fw_header_check_all();

	for(uint32_t i = 0; i < 4; i++)
		for(uint32_t k = 0; k < 3; k++)
			debounce_init(&btn[i][k], 2000);

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_APP); // let preboot know it was booted from bootloader

	adc_init();

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	usb_init();

	hbt_head = hb_tracker_init(RFM_NET_ID_HEAD, 700 /* 2x HB + 100 ms*/);

	air_protocol_init();
	air_protocol_init_node(iterator_head, RFM_NET_ID_HEAD);
	air_protocol_init_node(iterator_tail, RFM_NET_ID_TAIL);

	for(;;)
	{
		// uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		// static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		// // uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		// remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		// uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		// remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		poll_main();

		platform_watchdog_reset();

		air_protocol_poll();

		GPIOB->ODR ^= (1 << 10);

		static uint32_t prev_ticke = 0;
		if(prev_ticke < system_time_ms)
		{
			prev_ticke = system_time_ms + 100;
			led_drv_set_led(LED_7, hb_tracker_is_timeout(hbt_head) ? LED_MODE_ON : LED_MODE_SFLASH_SINGLE);
		}

		static int ear_move_step = -1;
		static uint32_t ear_move_delay = 0;

		if(ear_move_step != -1 && ear_move_delay < system_time_ms)
		{
			static uint8_t data_ear_smooth_servo[] = {RFM_NET_CMD_SERVO_SMOOTH, 0, 0,
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
			air_protocol_send_async(iterator_head, data_ear_smooth_servo, sizeof(data_ear_smooth_servo));

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
				uint8_t data[] = {RFM_NET_CMD_SERVO_RAW, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
				CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
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
				uint8_t data[] = {RFM_NET_CMD_FAN, 0, 0};
				data[1] = fan_hi[mode];
				data[2] = fan_lo[mode];
				CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
			}

			if(btn[1][1].state == BTN_PRESS_SHOT)
			{
				static const uint8_t leds[3][4] = {{0, 120, 0, 0},
												   {0, 0, 120, 0},
												   {0, 0, 0, 120}};
				static uint32_t mode = 0;
				mode++;
				if(mode >= 4) mode = 0;
				uint8_t data[] = {RFM_NET_CMD_LIGHT, 0, 0, 0};
				data[1] = leds[0][mode];
				data[2] = leds[1][mode];
				data[3] = leds[2][mode];
				CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
			}

			if(btn[2][1].state == BTN_PRESS_SHOT)
			{
				static const uint16_t servo_tongue[] = {320, /*470,*/ 620};
				static uint32_t mode = 0;
				mode++;
				if(mode >= sizeof(servo_tongue) / sizeof(servo_tongue[0])) mode = 0;
				uint8_t data[] = {RFM_NET_CMD_SERVO_RAW, 6, 0, 0};
				memcpy(&data[2], &servo_tongue[mode], 2);
				CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
			}

			static uint8_t data_smooth_servo[] = {RFM_NET_CMD_SERVO_SMOOTH, 0, 0,
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
				air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
			}

			if(btn[1][2].state == BTN_PRESS_SHOT)
			{
				uint16_t time = 800;
				memcpy(&data_smooth_servo[1], &time, 2);
				uint16_t pos[4] = {340, 380, 600, 500};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
			}

			if(btn[2][2].state == BTN_PRESS_SHOT)
			{
				uint16_t pos[4] = {500, 420, 550, 330};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
			}

			if(btn[3][2].state == BTN_PRESS_SHOT)
			{
				// uint16_t pos[4] = {440,330,640,400}; // alt
				uint16_t pos[4] = {500, 330, 620, 330};
				for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
					memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
				air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
			}
		}
	}
}

// static uint8_t dbg_buffer[RFM12B_MAXDATA];
void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
	// debug("RX: %d > Size: %d | %d %d %d %d\n", sender_node_id, data_len, *(data-4), *(data-3),*(data-2),*(data-1));
	// debug(DBG_INFO"R %d > %d\n", sender_node_id, data[0]);
	// if(data_len > 0)
	// {
	// 	switch(data[0])
	// 	{
	// 	case RFM_NET_CMD_DEBUG:
	// 		memcpy_volatile(dbg_buffer, data + 1, (size_t)(data_len - 1));
	// 		if(dbg_buffer[data_len - 2] != '\n')
	// 		{
	// 			dbg_buffer[data_len - 1] = '\n';
	// 			dbg_buffer[data_len - 0] = '\0';
	// 		}
	// 		else
	// 		{
	// 			dbg_buffer[data_len - 1] = '\0';
	// 		}
	// 		debug("[***]\t%s\n", dbg_buffer);
	// 		// for(uint8_t i = 1; i < data_len; i++)
	// 		// debug("%c", data[i]);
	// 		// if(data[data_len - 1] != '\n') debug("\n");
	// 		break;

	// 		// case RFM_NET_CMD_HB:
	// 		// {
	// 		//     if(data_len == 2)
	// 		//     {
	// 		//         bool not_found = hb_tracker_update(sender_node_id);
	// 		//         if(not_found) debug("HB unknown %d\n", sender_node_id);
	// 		//         if(sender_node_id == RFM_NET_ID_HEAD)
	// 		//         {
	// 		//             to_from_head = data[1];
	// 		//         }
	// 		//     }
	// 		// }
	// 		// break;

	// 	case RFM_NET_CMD_STS_HEAD:
	// 		if(data_len == 1 + 4 + 4)
	// 		{
	// 			float vbat, temp;
	// 			memcpy_volatile(&vbat, (data + 1), 4);
	// 			memcpy_volatile(&temp, (data + 1 + 4), 4);
	// 			debug(DBG_INFO "HD STS: %.2fV (%.1f%%) | t %.1f\n",
	// 				  vbat,
	// 				  map_float(vbat, 3.1f * 2.f, 4.18f * 2.f, 0, 100.f),
	// 				  temp);
	// 		}
	// 		break;

	// 	case RFM_NET_CMD_FLASH:
	// 	{
	// 		static uint8_t temp_data[CDC_DATA_FS_OUT_PACKET_SIZE];
	// 		uint32_t sz = data_len < CDC_DATA_FS_OUT_PACKET_SIZE ? data_len : CDC_DATA_FS_OUT_PACKET_SIZE;
	// 		for(uint32_t i = 0; i < sz; i++)
	// 		{
	// 			temp_data[i] = data[i];
	// 		}
	// 		CDC_Transmit_FS(temp_data, sz);
	// 	}
	// 	break;

	// 	default:
	// 		debug(DBG_WARN "Unknown cmd %d\n", data[0]);
	// 		break;
	// 	}
	// }
}