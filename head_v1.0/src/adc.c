#include "adc.h"
#include "debug.h"
#include "main.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;

static const float v_ref = 3.3f;
static const float adc_max_cnt = 4095.0f;

#define NTC_COEF 3434.0f
//NTC -> lo side
#define NTC_RES_LO_SIDE(adc_val) ((adc_val * 10000.0f) / (adc_max_cnt - adc_val))
// #define NTC_TEMP_LO_SIDE(adc_raw, coef) (1.0f / ((logf(NTC_RES_LO_SIDE(adc_raw) / 47000.0f) / coef) + (1.0 / 298.15f)) - 273.15f)
#define NTC_TEMP_LO_SIDE(adc_raw, coef) (1.0f / ((logf_fast(NTC_RES_LO_SIDE(adc_raw) / 47000.0f) / coef) + (1.0 / 298.15f)) - 273.15f)

inline float __int_as_float(int32_t a)
{
    union {
        int32_t a;
        float b;
    } u;
    u.a = a;
    return u.b;
}

inline int32_t __float_as_int(float b)
{
    union {
        int32_t a;
        float b;
    } u;
    u.b = b;
    return u.a;
}

/* natural log on [0x1.f7a5ecp-127, 0x1.fffffep127]. Maximum relative error 9.4529e-5 */
inline float logf_fast(float a)
{
    float m, r, s, t, i, f;
    int32_t e;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    e = (__float_as_int(a) - 0x3f2aaaab) & 0xff800000;
#pragma GCC diagnostic pop

    m = __int_as_float(__float_as_int(a) - e);
    i = (float)e * 1.19209290e-7f; // 0x1.0p-23
    /* m in [2/3, 4/3] */
    f = m - 1.0f;
    s = f * f;
    /* Compute log1p(f) for f in [-1/3, 1/3] */
    r = (0.230836749f * f - 0.279208571f); // 0x1.d8c0f0p-3, -0x1.1de8dap-2
    t = (0.331826031f * f - 0.498910338f); // 0x1.53ca34p-2, -0x1.fee25ap-2
    r = (r * s + t);
    r = (r * s + f);
    r = (i * 0.693147182f + r); // 0x1.62e430p-1 // log(2)
    return r;
}


float adc_logic_get_temp(void)
{
    return NTC_TEMP_LO_SIDE(adc_get_raw(ADC_SAIN_NTC0), NTC_COEF);
}

float adc_logic_get_vbat(void)
{
    return (float)adc_get_raw(ADC_SAIN_VBAT) / adc_max_cnt * v_ref * (1.0f + 47.0f / 10.0f);
}

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
        debug("ADC: %d %d\n", i, adc_get_raw(i));
    }
    debug("Temp: %.1f\n", adc_logic_get_temp());
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
