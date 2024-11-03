#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>
#include <stdint.h>

enum
{
	SERVO_EYE_R_FAR = 0,
	SERVO_EYE_R_NEAR,
	SERVO_EAR_R,
	SERVO_EAR_L,
	SERVO_EYE_L_NEAR,
	SERVO_EYE_L_FAR,
	SERVO_TONGUE,
	SERVO_COUNT, // must have same count as adc_val.s_pos[]
};

void servo_init(void);

void servo_set(uint8_t servo, uint32_t value);
void servo_set_smooth_and_off(uint8_t servo, uint32_t pos_end, uint32_t delay_ms);
void servo_poll(uint32_t diff_ms);

void servo_print(void);

#endif // SERVO_H