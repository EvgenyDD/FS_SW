#ifndef ADC_H
#define ADC_H

#include <stdint.h>

enum
{
    ADC_SAIN_0,
    ADC_SAIN_1,
    ADC_SAIN_2,
    ADC_SAIN_3,
    ADC_SAIN_4,
    ADC_SAIN_5,
    ADC_SAIN_VBAT,
    ADC_SAIN_NTC0,
    ADC_SAIN_RSSI,

    ADC_CH_NUM
};

void adc_init(void);
void adc_print(void);

uint16_t adc_get_raw(uint32_t channel);

float adc_logic_get_temp(void);
float adc_logic_get_vbat(void);

void adc_drv_conv_complete_half(void);
void adc_drv_conv_complete_full(void);

#endif // ADC_H