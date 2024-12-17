#include "servo.h"
#include "adc.h"
#include "console.h"
#include "debug.h"
#include "platform.h"
#include <string.h>

#define MAX_POINTS 64

typedef enum
{
	MODE_OFF = 0,
	MODE_MOVE,
	MODE_DELAY_BEFORE_OFF,
} servo_mode_t;

static struct
{
	servo_mode_t mode;
	uint16_t pos_end;
	uint16_t pos_start;
	uint16_t pos_last;
	uint32_t time_start;
	uint32_t time_print_state;
	uint32_t time_end;
} servo_move[SERVO_COUNT] = {0};

static __IO uint32_t *const ptr[SERVO_COUNT] = {&TIM1->CCR1, &TIM1->CCR2, &TIM1->CCR3, &TIM1->CCR4, &TIM4->CCR1, &TIM4->CCR2, &TIM4->CCR3};

static uint16_t get_approximation(uint8_t servo, float adc)
{
	if(adc < 16) return servo_move[servo].pos_last;
	switch(servo)
	{
	case SERVO_EAR_R: return adc * 1.604f - 89.163f;
	case SERVO_EAR_L: return adc * 1.43534f + 3.56723f;
	case SERVO_EYE_L_NEAR: return adc * (1.42778f + 1.39f) * 0.5f + (14.043f - 0.9757f) * 0.5f;
	case SERVO_EYE_L_FAR: return adc * (1.409092f + 1.4254167f) * 0.5f + (10.18966f - 8.4515f) * 0.5f;
	case SERVO_EYE_R_FAR: return adc * (1.3478f + 1.52054f) * 0.5f + (24.697f - 53.6587f) * 0.5f;
	case SERVO_EYE_R_NEAR: return adc * (1.339815f + 1.3266f) * 0.5f + (27.574f + 6.81544f) * 0.5f;
	default: return servo_move[servo].pos_last;
	}
}

uint32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
	if(x < in_min) x = in_min;
	if(x > in_max) x = in_max;
	return (uint32_t)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

void servo_init(void)
{
	memset(servo_move, 0, sizeof(servo_move));
	servo_move[SERVO_EYE_R_FAR].pos_last = 420;
	servo_move[SERVO_EYE_R_NEAR].pos_last = 445;
	servo_move[SERVO_EAR_R].pos_last = 550;
	servo_move[SERVO_EAR_L].pos_last = 475;
	servo_move[SERVO_EYE_L_NEAR].pos_last = 495;
	servo_move[SERVO_EYE_L_FAR].pos_last = 430;
	servo_move[SERVO_TONGUE].pos_last = 470;
}

void servo_set_smooth_and_off(uint8_t servo, uint32_t pos_end, uint32_t delay_ms)
{
	if(servo >= SERVO_COUNT) return;

	uint16_t pos_start = get_approximation(servo, adc_val.s_pos[servo]);
	servo_move[servo].pos_start = pos_start != 0 ? pos_start : servo_move[servo].pos_last;
	servo_move[servo].pos_end = pos_end;
	servo_move[servo].time_start = system_time_ms;
	servo_move[servo].time_print_state = servo_move[servo].time_start + 250;
	servo_move[servo].time_end = servo_move[servo].time_start + delay_ms;
	servo_move[servo].mode = MODE_MOVE;
}

void servo_set(uint8_t servo, uint32_t value)
{
	uint32_t v = map((int32_t)value, 0, 1000, 400, 5600);
	if(value == 0) v = 0;
	if(servo >= SERVO_COUNT) return;
	*ptr[servo] = v;
}

void servo_print(void)
{
	_console_print("Servo0: %ld\n", TIM1->CCR1 / 2);
	_console_print("Servo1: %ld\n", TIM1->CCR2 / 2);
	_console_print("Servo2: %ld\n", TIM1->CCR3 / 2);
	_console_print("Servo3: %ld\n", TIM1->CCR4 / 2);
	_console_print("Servo4: %ld\n", TIM4->CCR1 / 2);
	_console_print("Servo5: %ld\n", TIM4->CCR2 / 2);
	_console_print("Servo6: %ld\n", TIM4->CCR3 / 2);
}

void servo_poll(uint32_t diff_ms)
{
	if(diff_ms > 0)
	{
		uint32_t time_now = system_time_ms;
		for(uint32_t i = 0; i < SERVO_COUNT; i++)
		{
			if(servo_move[i].mode == MODE_MOVE)
			{
				if(servo_move[i].time_end <= time_now)
				{
					servo_set(i, servo_move[i].pos_end);
					servo_move[i].pos_last = servo_move[i].pos_end;
					servo_move[i].time_end += 50; // delay before off
					servo_move[i].mode = MODE_DELAY_BEFORE_OFF;
				}
				else
				{
					uint16_t pos = (int32_t)servo_move[i].pos_start + ((int32_t)servo_move[i].pos_end - (int32_t)servo_move[i].pos_start) *
																		  (int32_t)(time_now - servo_move[i].time_start) /
																		  (int32_t)(servo_move[i].time_end - servo_move[i].time_start);
					servo_move[i].pos_last = pos;
					servo_set(i, pos);
					if(servo_move[i].time_print_state < time_now)
					{
						// _console_print("%d>%.1f|%.1f|%.1f|%.1f|%.1f|%.1f|(%.1f)", pos, adc_val.s_pos[0], adc_val.s_pos[1], adc_val.s_pos[2], adc_val.s_pos[3], adc_val.s_pos[4], adc_val.s_pos[5], adc_val.s_pos[6]);
						servo_move[i].time_print_state = time_now + 180;
					}
				}
			}
			else if(servo_move[i].mode == MODE_DELAY_BEFORE_OFF)
			{
				if(servo_move[i].time_end < time_now)
				{
					servo_set(i, 0);
					servo_move[i].mode = MODE_OFF;
				}
			}
		}
	}
}
