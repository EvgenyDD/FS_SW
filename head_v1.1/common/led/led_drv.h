#ifndef LED_DRV_H
#define LED_DRV_H

#include "led_drv.h"
#include "platform.h"
#include <stdbool.h>
#include <stdint.h>

enum LEDS
{
	LED_0 = 0,
	LED_COUNT
};

typedef enum
{
	LED_MODE_OFF = 0,
	LED_MODE_ON,

	LED_MODE_FLASH_10HZ,	// 10. Hz [50 ms ON + 50 ms OFF]
	LED_MODE_FLASH_2_5HZ,	// 2.5 Hz [200 ms ON + 200 ms OFF]
	LED_MODE_FLASH_SINGLE,	// [200 ms ON + 1000 ms OFF]
	LED_MODE_FLASH_DOUBLE,	// [200 ms ON + 200 ms OFF + 200 ms ON + 1000 ms OFF]
	LED_MODE_FLASH_TRIPLE,	// [200 ms ON + 200 ms OFF + 200 ms ON + 200 ms OFF + 200 ms ON + 1000 ms OFF]
	LED_MODE_SFLASH_2_5HZ,	// SMOOTH 2.5 Hz [200 ms ON + 200 ms OFF]
	LED_MODE_SFLASH_SINGLE, // SMOOTH [200 ms ON + 1000 ms OFF]
	LED_MODE_SFLASH_DOUBLE, // SMOOTH [200 ms ON + 200 ms OFF + 200 ms ON + 1000 ms OFF]
	LED_MODE_SFLASH_TRIPLE, // SMOOTH [200 ms ON + 200 ms OFF + 200 ms ON + 200 ms OFF + 200 ms ON + 1000 ms OFF]

	LED_MODE_MANUAL,

	LED_MODE_SIZE
} LED_MODE;

void led_drv_poll(uint32_t diff_ms);

void led_drv_set_led(uint32_t led_id, LED_MODE mode);
void led_drv_set_led_manual(uint32_t led_id, float brightness);

float interval_hit(int32_t value, int32_t middle, int32_t half_sector, int32_t range);

#endif // LED_DRV_H