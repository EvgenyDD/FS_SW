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

    SERVO_COUNT,
};

void servo_init(void);

bool servo_set(uint8_t servo, uint32_t value);
bool servo_set_smooth_and_off(uint8_t servo, uint32_t pos_end, uint32_t delay_ms);
void servo_poll(uint32_t diff_ms);

void servo_print(void);

typedef struct
{
    uint32_t start_time;
    uint32_t stop_time;

    uint32_t start_pos;
    uint32_t end_pos;
} move_point_t;

#endif // SERVO_H