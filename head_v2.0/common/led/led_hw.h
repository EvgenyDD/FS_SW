#ifndef LED_HW_H__
#define LED_HW_H__

enum LEDS
{
	LED_0 = 0,
	LED_COUNT
};

#define LED_HW_DRIVE() \
	cnt < led_pwm_lvl[LED_0] ? (GPIOC->BSRRL = (1 << 0)) : (GPIOC->BSRRH = (1 << 0));

#endif // LED_HW_H__