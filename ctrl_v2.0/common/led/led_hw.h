#ifndef LED_HW_H__
#define LED_HW_H__

enum LEDS
{
	LED_0 = 0,
	LED_1,
	LED_2,
	LED_3,
	LED_4,
	LED_5,
	LED_6,
	LED_7,
	LED_COUNT
};

#define LED_HW_DRIVE()                                                                  \
	cnt < led_pwm_lvl[LED_0] ? (GPIOA->BSRRL = (1 << 7)) : (GPIOA->BSRRH = (1 << 7));   \
	cnt < led_pwm_lvl[LED_1] ? (GPIOC->BSRRL = (1 << 4)) : (GPIOC->BSRRH = (1 << 4));   \
	cnt < led_pwm_lvl[LED_2] ? (GPIOC->BSRRL = (1 << 5)) : (GPIOC->BSRRH = (1 << 5));   \
	cnt < led_pwm_lvl[LED_3] ? (GPIOB->BSRRL = (1 << 0)) : (GPIOB->BSRRH = (1 << 0));   \
	cnt < led_pwm_lvl[LED_4] ? (GPIOB->BSRRL = (1 << 1)) : (GPIOB->BSRRH = (1 << 1));   \
	cnt < led_pwm_lvl[LED_5] ? (GPIOB->BSRRL = (1 << 2)) : (GPIOB->BSRRH = (1 << 2));   \
	cnt < led_pwm_lvl[LED_6] ? (GPIOB->BSRRL = (1 << 10)) : (GPIOB->BSRRH = (1 << 10)); \
	cnt < led_pwm_lvl[LED_7] ? (GPIOB->BSRRL = (1 << 11)) : (GPIOB->BSRRH = (1 << 11));

#endif // LED_HW_H__