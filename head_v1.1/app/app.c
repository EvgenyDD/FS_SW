#include "adc.h"
#include "air_protocol.h"
#include "config_system.h"
#include "crc.h"
#include "debounce.h"
#include "debug.h"
#include "fw_header.h"
#include "hb_tracker.h"
#include "led.h"
#include "led_drv.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "servo.h"
#include <stddef.h>
#include <string.h>

#define MASTER_TO_MS_SHUTDOWN 10000

#define SYSTICK_IN_US (48000000 / 1000000)
#define SYSTICK_IN_MS (48000000 / 1000)

volatile uint32_t system_time_ms = 0;

static int32_t prev_systick = 0;

config_entry_t g_device_config[] = {};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

uint32_t g_uid[3];

const uint8_t iterator_ctrl = 0;

static hbt_node_t *hbt_ctrl;

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

	servo_init();

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_BOOTLOADER); // let preboot know it was booted from bootloader

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	debounce_init(&btn_pwr, 2000);
	btn_pwr.int_state_prev = btn_pwr.int_state = btn_pwr.state = BTN_PRESS;

	hbt_ctrl = hb_tracker_init(RFM_NET_ID_CTRL, 700 /* 2x HB + 100 ms*/);

	air_protocol_init();
	air_protocol_init_node(iterator_ctrl, RFM_NET_ID_CTRL);

	for(;;)
	{
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		// uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		platform_watchdog_reset();

		adc_track();

		led_drv_poll(diff_ms);

		system_time_ms += diff_ms;

		debounce_update(&btn_pwr, GPIOA->IDR & (1 << 12), diff_ms);

		if(btn_pwr.state == BTN_UNPRESS_SHOT) pwr_off();

		// master timeout
		{
			static uint32_t master_to = 0;

			if(hb_tracker_is_timeout(hbt_ctrl))
				master_to += diff_ms;
			else
				master_to = 0;

			if(master_to > MASTER_TO_MS_SHUTDOWN) pwr_off();
		}

		// low vbat
		{
			if(adc_val.v_bat < 3.1f * 2.f)
			{
				led_set(0, 200);
				led_set(1, 0);
				led_set(2, 0);

				for(uint32_t i = 0; i < 3; i++)
				{
					led_set(0, 0);
					delay_ms(500);

					led_set(0, 200);
					delay_ms(500);
				}
				pwr_off();
			}
		}

		air_protocol_poll();

		servo_poll(diff_ms);

		led_drv_set_led(LED_0, hb_tracker_is_timeout(hbt_ctrl) ? LED_MODE_ON : LED_MODE_SFLASH_2_5HZ);

		static uint32_t ctr_sts_head = 0;
		if(ctr_sts_head < system_time_ms)
		{
			ctr_sts_head = system_time_ms + 1000;

			uint8_t data[1 + 4 + 4] = {RFM_NET_CMD_STS_HEAD};
			float vbat = adc_val.v_bat;
			float temp = adc_val.t_ntc[0];
			// float temp2 = adc_val.t_ntc[1];
			memcpy(data + 1, &vbat, 4);
			memcpy(data + 1 + 4, &temp, 4);
			air_protocol_send_async(iterator_ctrl, data, sizeof(data));
			// debug_rf("Hi!");
			// debug_rf("*");
		}
	}
}

static void memcpy_volatile(void *dst, const volatile void *src, size_t size)
{
	for(uint32_t i = 0; i < size; i++)
	{
		*((uint8_t *)dst + i) = *((const volatile uint8_t *)src + i);
	}
}

static uint32_t addr_flash, len_flash;
static uint8_t tx_buf[64];

enum
{
	SSP_FLASH_OK = 0,
	SSP_FLASH_WRONG_PARAM,
	SSP_FLASH_UNEXIST,
	SSP_FLASH_FAIL,
	SSP_FLASH_WRONG_ADDRESS,
	SSP_FLASH_VERIFY_FAIL,
	SSP_FLASH_WRONG_CRC_ADDRESS,
	SSP_FLASH_WRONG_FULL_LENGTH,
	SSP_FLASH_WRONG_CRC,
};

#define ADDR_APP_IMAGE 0
#define LEN_APP_IMAGE 0x40000

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
	// debug("RX: %d > Size: %d\n", sender_node_id, data_len);if(data_len > 0)
	{
		switch(data[0])
		{
			// case RFM_NET_CMD_HB:
			// {
			//     if(data_len == 2)
			//     {
			//         bool not_found = hb_tracker_update(sender_node_id);
			//         // if(not_found) debug("HB unknown %d\n", sender_node_id);
			//     }
			// }
			// break;

		case RFM_NET_CMD_LIGHT:
			if(data_len == 4)
			{
				led_set_gamma(0, data[1]);
				led_set_gamma(1, data[2]);
				led_set_gamma(2, data[3]);
			}
			break;

		case RFM_NET_CMD_FAN:
			if(data_len == 3)
			{
				fan_set(0, data[1]);
				fan_set(1, data[2]);
			}
			break;

		case RFM_NET_CMD_SERVO_RAW:
			if(data_len == 1 + SERVO_COUNT * 2) // all servos
			{
				for(uint32_t i = 0; i < SERVO_COUNT; i++)
				{
					uint16_t signal;
					memcpy_volatile(&signal, &data[1U + 2U * i], 2);
					servo_set(i, signal);
				}
			}
			else if(data_len == 1 + 1 /* selector */ + 2 /* data */) // single servo
			{
				if(data[1] < SERVO_COUNT)
				{
					uint16_t signal;
					memcpy_volatile(&signal, &data[2], 2);
					servo_set(data[1], signal);
				}
			}
			break;

		case RFM_NET_CMD_SERVO_SMOOTH:
			if(data_len >= 1 + 2 /* time */ + 1 /* selector */ + 2 /* data */)
			{
				if(((data_len - 3) % 3) == 0)
				{
					uint16_t time;
					memcpy_volatile(&time, &data[1], 2);
					for(uint32_t i = 3; i < data_len;)
					{
						uint16_t pos;
						memcpy_volatile(&pos, &data[i + 1], 2);
						servo_set_smooth_and_off(data[i], pos, time);
						i += 3;
					}
				}
			}
			break;

		case RFM_NET_CMD_REBOOT:
			NVIC_SystemReset();
			break;

		case RFM_NET_CMD_DEBUG:
			if(data_len >= 3) // at least 1 symbol
			{
				if(data[1] == RFM_NET_ID_HEAD)
				{
					for(uint32_t i = 2; i < data_len; i++)
						debug_rx(data[i]);
				}
			}
			break;

		case RFM_NET_CMD_OFF:
			if(data_len >= 2)
			{
				if(data[1] == RFM_NET_ID_HEAD) pwr_off();
			}
			break;

		case RFM_NET_CMD_FLASH:
		{
			tx_buf[0] = RFM_NET_CMD_FLASH;
			tx_buf[1] = data[1];
			if(data_len >= 7) // at least 1 symbol
			{
				if(((data_len - 6) % 4) == 0)
				{
					len_flash = (uint32_t)(data_len - 6);
					memcpy_volatile(&addr_flash, data + 2, 4);

					switch(data[1])
					{
					case RFM_NET_ID_HEAD:
					{
						if(addr_flash < ADDR_APP_IMAGE || addr_flash >= ADDR_APP_IMAGE + LEN_APP_IMAGE)
						{
							tx_buf[2] = SSP_FLASH_WRONG_ADDRESS;
						}
						else
						{
							tx_buf[2] = SSP_FLASH_OK;
							if(addr_flash == ADDR_APP_IMAGE)
							{
								len_flash = 4;
								if(data_len != 6 + 4 /*data*/ + 4 /* len */ + 4 /*crc*/)
								{
									tx_buf[2] = SSP_FLASH_WRONG_CRC_ADDRESS;
								}
								else
								{
									uint32_t first_word, len_full, crc_ref;
									memcpy_volatile(&first_word, data + 6, 4);
									memcpy_volatile(&len_full, data + 6 + 4, 4);
									memcpy_volatile(&crc_ref, data + 6 + 4 + 4, 4);

									if(len_full >= LEN_APP_IMAGE || (len_full % 4) != 0)
									{
										tx_buf[2] = SSP_FLASH_WRONG_FULL_LENGTH;
									}
									else
									{
										CRC->CR |= CRC_CR_RESET;
										CRC->DR = first_word;
										for(uint32_t i = ADDR_APP_IMAGE + 4; i < (ADDR_APP_IMAGE + len_full); i += 4)
										{
											CRC->DR = *(uint32_t *)i;
										}
										if(CRC->DR != crc_ref)
										{
											tx_buf[2] = SSP_FLASH_WRONG_CRC;
										}
									}
								}
							}
							if(tx_buf[2] == SSP_FLASH_OK)
							{
								for(uint32_t i = 0; i < len_flash; i++)
								{
									if(FLASH_ProgramByte(addr_flash + i, data[6 + i]) != FLASH_COMPLETE)
									{
										tx_buf[2] = SSP_FLASH_FAIL;
										break;
									}
									if(*(uint8_t *)(addr_flash + i) != data[6 + i])
									{
										tx_buf[2] = SSP_FLASH_VERIFY_FAIL;
										break;
									}
								}
							}

							memcpy(tx_buf + 3, &addr_flash, 4);
							air_protocol_send_async(iterator_ctrl, tx_buf, 3 + 4);
							break;
						}

						air_protocol_send_async(iterator_ctrl, tx_buf, 3);
					}
					break;

					default:
					{
						tx_buf[2] = SSP_FLASH_UNEXIST;
						air_protocol_send_async(iterator_ctrl, tx_buf, 3);
					}
					break;
					}
				}
				else
				{
					tx_buf[2] = SSP_FLASH_WRONG_PARAM;
					tx_buf[3] = data_len;
					air_protocol_send_async(iterator_ctrl, tx_buf, 4);
				}
			}
			else
			{
				tx_buf[2] = SSP_FLASH_WRONG_PARAM;
				tx_buf[3] = data_len;
				air_protocol_send_async(iterator_ctrl, tx_buf, 4);
			}
		}
		break;

		default:
			// debug("Unknown cmd %d\n", data[0]);
			break;
		}
	}
}