#include "servo.h"
#include "debug.h"
#include "main.h"

#include <string.h>

#include "adc.h" // for debug

#define MAX_POINTS 64
move_point_t points[SERVO_COUNT][MAX_POINTS];
uint32_t points_push[SERVO_COUNT];
uint32_t points_pop[SERVO_COUNT];
uint32_t points_timer[SERVO_COUNT];

typedef enum
{
    MODE_OFF = 0,
    MODE_MOVE,
    MODE_DELAY_BEFORE_OFF,
} Servo_mode;

typedef struct
{
    Servo_mode mode;
    uint16_t pos_end;
    uint16_t pos_start;
    uint16_t pos_last;
    uint32_t time_start;
    uint32_t time_print_state; // for debug only
    uint32_t time_end;
} servo_t;

servo_t servo_move[SERVO_COUNT];

static __IO uint32_t *const ptr[SERVO_COUNT] = {
    &TIM1->CCR1,
    &TIM1->CCR2,
    &TIM1->CCR3,
    &TIM1->CCR4,
    &TIM4->CCR1,
    &TIM4->CCR2,
    &TIM4->CCR3};

static inline uint16_t get_approximation(float servo, float adc)
{
    /**/ if(servo == SERVO_EAR_R)
        return adc * 1.604f - 89.163f;
    else if(servo == SERVO_EAR_L)
        return adc * 1.43534f + 3.56723f;
    else if(servo == SERVO_EYE_L_NEAR)
        return adc * (1.42778f + 1.39f) * 0.5f + (14.043f - 0.9757f) * 0.5f;
    else if(servo == SERVO_EYE_L_FAR)
        return adc * (1.409092f + 1.4254167f) * 0.5f + (10.18966f - 8.4515f) * 0.5f;
    else if(servo == SERVO_EYE_R_FAR)
        return adc * (1.3478f + 1.52054f) * 0.5f + (24.697f - 53.6587f) * 0.5f;
    else if(servo == SERVO_EYE_R_NEAR)
        return adc * (1.339815f + 1.3266f) * 0.5f + (27.574f + 6.81544f) * 0.5f;

    return 0;
}

uint32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    if(x < in_min)
    {
        x = in_min;
    }
    if(x > in_max)
    {
        x = in_max;
    }
    return (uint32_t)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

void servo_init(void)
{
    memset(points, 0, sizeof(points));
    memset(points_push, 0, sizeof(points_push));
    memset(points_pop, 0, sizeof(points_pop));
    memset(points_timer, 0, sizeof(points_timer));

    memset(servo_move, 0, sizeof(servo_move));
    servo_move[SERVO_EYE_R_FAR].pos_last = 420;
    servo_move[SERVO_EYE_R_NEAR].pos_last = 445;
    servo_move[SERVO_EAR_R].pos_last = 550;
    servo_move[SERVO_EAR_L].pos_last = 475;
    servo_move[SERVO_EYE_L_NEAR].pos_last = 495;
    servo_move[SERVO_EYE_L_FAR].pos_last = 430;
    servo_move[SERVO_TONGUE].pos_last = 470;
}

bool servo_set_smooth_and_off(uint8_t servo, uint32_t pos_end, uint32_t delay_ms)
{
    if(servo >= SERVO_COUNT) return true;

    uint16_t pos_start = get_approximation(servo, adc_get_raw(servo));
    servo_move[servo].pos_start = pos_start != 0 ? pos_start : servo_move[servo].pos_last;
    servo_move[servo].pos_end = pos_end;
    servo_move[servo].time_start = HAL_GetTick();
    servo_move[servo].time_print_state = servo_move[servo].time_start + 250;
    servo_move[servo].time_end = servo_move[servo].time_start + delay_ms;
    servo_move[servo].mode = MODE_MOVE;

    return false;
}

bool servo_set(uint8_t servo, uint32_t value)
{
    uint32_t v = map((int32_t)value, 0, 1000, 400, 5600);
    if(value == 0) v = 0;

    if(servo >= SERVO_COUNT) return true;
    *ptr[servo] = v;
    return false;
    // switch(servo)
    // {
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // default: return true;
    // }
    // if(servo == 0)
    //     TIM1->CCR1 = v;
    // else if(servo == 1)
    // {
    //     TIM1->CCR2 = v;
    //     return false;
    // }
    // else if(servo == 2)
    // {
    //     TIM1->CCR3 = v;
    //     return false;
    // }
    // else if(servo == 3)
    // {
    //     TIM1->CCR4 = v;
    //     return false;
    // }
    // else if(servo == 4)
    // {
    //     TIM4->CCR1 = v;
    //     return false;
    // }
    // else if(servo == 5)
    // {
    //     TIM4->CCR2 = v;
    //     return false;
    // }
    // else if(servo == 6)
    // {
    //     TIM4->CCR3 = v;
    //     return false;
    // }
    // return true;
}

void servo_print(void)
{
    debug("Servo: \n\t 0\t%d\n\t 1\t%d\n\t 2\t%d\n\t 3\t%d\n\t 4\t%d\n\t 5\t%d\n\t 6\t%d\n",
          TIM1->CCR1 / 2,
          TIM1->CCR2 / 2,
          TIM1->CCR3 / 2,
          TIM1->CCR4 / 2,
          TIM4->CCR1 / 2,
          TIM4->CCR2 / 2,
          TIM4->CCR3 / 2);
}

void servo_poll(uint32_t diff_ms)
{
    if(diff_ms > 0)
    {
        uint32_t time_now = HAL_GetTick();
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
                        debug_rf("%d>%d|%d|%d|%d|%d|%d", pos,
                                 adc_get_raw(ADC_SAIN_0),
                                 adc_get_raw(ADC_SAIN_1),
                                 adc_get_raw(ADC_SAIN_2),
                                 adc_get_raw(ADC_SAIN_3),
                                 adc_get_raw(ADC_SAIN_4),
                                 adc_get_raw(ADC_SAIN_5));
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
