#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

bool led_set(uint8_t led, uint32_t value);
bool led_set_gamma(uint8_t led, uint8_t value);
void led_set_hue(float h, float s, float v);

#endif // LED_H
