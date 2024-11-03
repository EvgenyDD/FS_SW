#include "fan.h"
#include "main.h"

bool fan_set(uint8_t fan, uint32_t value)
{
    if(value > 99) value = 99;
    __IO uint32_t *ptr[] = {
        &TIM12->CCR1,
        &TIM12->CCR2};
    if(fan > 1) return true;
    *ptr[fan] = value;
    return false;
}