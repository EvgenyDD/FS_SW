#ifndef POWER_H
#define POWER_H

#include "main.h"

#include <stdbool.h>

inline bool is_btn_pressed(void) { return BTN_SNS_GPIO_Port->IDR & BTN_SNS_Pin ? 1 : 0; }

inline void pwr_enable(void) { PWR_EN_GPIO_Port->ODR |= PWR_EN_Pin;}
inline void pwr_disable(void) { PWR_EN_GPIO_Port->ODR &= (uint32_t)(~PWR_EN_Pin);}

#endif // POWER_H