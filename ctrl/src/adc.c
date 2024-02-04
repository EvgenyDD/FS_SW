#include "adc.h"
#include "debug.h"
#include "main.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;

static const float v_ref = 3.3f;
static const float adc_max_cnt = 4095.0f;

enum
{
    ADC_SAIN_VBAT,

    ADC_CH_NUM
};
uint32_t adc_sel = 0; // offset, values: 0 or ADC_CH_NUM
static uint16_t adc_raw[ADC_CH_NUM * 2];

void adc_init(void)
{
    HAL_ADC_Start(&hadc1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_raw, ADC_CH_NUM * 2);
#pragma GCC diagnostic pop
    __HAL_TIM_ENABLE(&htim3);
}

void adc_print(void)
{
    for(uint32_t i = 0; i < ADC_CH_NUM; i++)
    {
        debug("ADC #%d: %d\n", i, adc_get_raw(i));
    }
}

uint16_t adc_get_raw(uint32_t channel) { return adc_raw[adc_sel + channel]; }

void adc_drv_conv_complete_half(void)
{
    adc_sel = 0;
    // filter_values();
}

void adc_drv_conv_complete_full(void)
{
    adc_sel = ADC_CH_NUM;
    // filter_values();
}

float adc_drv_get_vbat(void) { return (float)adc_get_raw(ADC_SAIN_VBAT) / 4095.f * 3.3f * (1.f + 1000.f / 470.f) * 1.02202643f; }
