#include "adc.h"
#include "debounce.h"
#include "debug.h"
#include "fan.h"
#include "flash_regions.h"
#include "flasher_hal.h"
#include "hb_tracker.h"
#include "led.h"
#include "main.h"
#include "power.h"
#include "serial_suit_protocol.h"
#include "servo.h"

#include <string.h>

#define MASTER_TO_MS_SHUTDOWN 10000

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim12;

extern UART_HandleTypeDef huart3;

extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;

#warning "Add WDT"

hbt_node_t *hbt_ctrl;

uint32_t get_random(void) { return HAL_RNG_GetRandomNumber(&hrng); }

const uint8_t iterator_ctrl = 0;

static button_ctrl_t btn_pwr;

void init(void)
{
    debounce_init(&btn_pwr, 200, 1000);
    btn_pwr.pressed = true;

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_3);
    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);

    __HAL_UART_ENABLE(&huart3);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    adc_init();
    servo_init();

    pwr_enable();

    hbt_ctrl = hb_tracker_init(RFM_NET_ID_CTRL, 700 /* 2x HB + 100 ms*/);

    air_protocol_init();
    air_protocol_init_node(iterator_ctrl, RFM_NET_ID_CTRL);
}

void loop(void)
{
    // time diff
    static uint32_t time_ms_prev = 0;
    uint32_t time_ms_now = HAL_GetTick();
    uint32_t diff_ms = time_ms_now < time_ms_prev
                           ? 0xFFFFFFFF + time_ms_now - time_ms_prev
                           : time_ms_now - time_ms_prev;
    time_ms_prev = time_ms_now;

    static uint32_t prev_tick = 0;
    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
    }

    // btn
    {
        static bool was_unpressed = false;
        debounce_cb(&btn_pwr, is_btn_pressed(), diff_ms);
        if(btn_pwr.pressed_shot && was_unpressed) pwr_disable();
        if(btn_pwr.unpressed_shot) was_unpressed = true;
    }

    // master timeout
    {
        static uint32_t master_to = 0;

        if(hb_tracker_is_timeout(hbt_ctrl))
            master_to += diff_ms;
        else
            master_to = 0;

        if(master_to > MASTER_TO_MS_SHUTDOWN) pwr_disable();
    }

    // low vbat
    {
        if(adc_logic_get_vbat() < 3.1f * 2.f)
        {
            led_set(0, 200);
            led_set(1, 0);
            led_set(2, 0);

            for(uint32_t i = 0; i < 3; i++)
            {
                led_set(0, 0);
                HAL_Delay(500);

                led_set(0, 200);
                HAL_Delay(500);
            }
            pwr_disable();
        }
    }

    air_protocol_poll();

    servo_poll(diff_ms);

    static uint32_t hbtt = 0;
    if(hbtt < HAL_GetTick())
    {
        hbtt = HAL_GetTick() + 100;

        if(hb_tracker_is_timeout(hbt_ctrl))
        {
            LED_GPIO_Port->ODR |= LED_Pin;
        }
        else
        {
            LED_GPIO_Port->ODR ^= LED_Pin;
        }
    }

    static uint32_t ctr_sts_head = 0;
    if(ctr_sts_head < HAL_GetTick())
    {
        ctr_sts_head = HAL_GetTick() + 1000;

        uint8_t data[1 + 4 + 4] = {RFM_NET_CMD_STS_HEAD};
        float vbat = adc_logic_get_vbat();
        float temp = adc_logic_get_temp();
        memcpy(data + 1, &vbat, 4);
        memcpy(data + 1 + 4, &temp, 4);
        air_protocol_send_async(iterator_ctrl, data, sizeof(data));
        // debug_rf("Hi!");
        // debug_rf("*");
    }
}

static uint32_t crc32_calc(const uint32_t *buf, uint32_t len)
{
    CRC->CR |= CRC_CR_RESET;

    for(uint32_t i = 0; i < len; i++)
    {
        CRC->DR = buf[i];
    }

    return CRC->DR;
}

inline static void memcpy_volatile(void *dst, const volatile void *src, size_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        *((uint8_t *)dst + i) = *((const volatile uint8_t *)src + i);
    }
}

static uint32_t addr_flash, len_flash;
static uint8_t tx_buf[64];

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
    // debug("RX: %d > Size: %d\n", sender_node_id, data_len);if(data_len > 0)
    {
        switch(data[0])
        {
            // case RFM_NET_CMD_HB:
            // {
            //     if(data_len == 2)
            //     {
            //         bool not_found = hb_tracker_update(sender_node_id);
            //         // if(not_found) debug("HB unknown %d\n", sender_node_id);
            //     }
            // }
            // break;

        case RFM_NET_CMD_LIGHT:
            if(data_len == 4)
            {
                led_set_gamma(0, data[1]);
                led_set_gamma(1, data[2]);
                led_set_gamma(2, data[3]);
            }
            break;

        case RFM_NET_CMD_FAN:
            if(data_len == 3)
            {
                fan_set(0, data[1]);
                fan_set(1, data[2]);
            }
            break;

        case RFM_NET_CMD_SERVO_RAW:
            if(data_len == 1 + SERVO_COUNT * 2) // all servos
            {
                for(uint32_t i = 0; i < SERVO_COUNT; i++)
                {
                    uint16_t signal;
                    memcpy_volatile(&signal, &data[1U + 2U * i], 2);
                    servo_set(i, signal);
                }
            }
            else if(data_len == 1 + 1 /* selector */ + 2 /* data */) // single servo
            {
                if(data[1] < SERVO_COUNT)
                {
                    uint16_t signal;
                    memcpy_volatile(&signal, &data[2], 2);
                    servo_set(data[1], signal);
                }
            }
            break;

        case RFM_NET_CMD_SERVO_SMOOTH:
            if(data_len >= 1 + 2 /* time */ + 1 /* selector */ + 2 /* data */)
            {
                if(((data_len - 3) % 3) == 0)
                {
                    uint16_t time;
                    memcpy_volatile(&time, &data[1], 2);
                    for(uint32_t i = 3; i < data_len;)
                    {
                        uint16_t pos;
                        memcpy_volatile(&pos, &data[i + 1], 2);
                        servo_set_smooth_and_off(data[i], pos, time);
                        i += 3;
                    }
                }
            }
            break;

        case RFM_NET_CMD_REBOOT:
            HAL_NVIC_SystemReset();
            break;

        case RFM_NET_CMD_DEBUG:
            if(data_len >= 3) // at least 1 symbol
            {
                if(data[1] == RFM_NET_ID_HEAD)
                {
                    for(uint32_t i = 2; i < data_len; i++)
                        debug_rx(data[i]);
                }
            }
            break;

        case RFM_NET_CMD_OFF:
            if(data_len >= 2)
            {
                if(data[1] == RFM_NET_ID_HEAD)
                {
                    pwr_disable();
                }
            }
            break;

        case RFM_NET_CMD_FLASH:
        {
            tx_buf[0] = RFM_NET_CMD_FLASH;
            tx_buf[1] = data[1];
            if(data_len >= 7) // at least 1 symbol
            {
                if(((data_len - 6) % 4) == 0)
                {
                    len_flash = (uint32_t)(data_len - 6);
                    memcpy_volatile(&addr_flash, data + 2, 4);

                    switch(data[1])
                    {
                    case RFM_NET_ID_HEAD:
                    {
                        if(addr_flash < ADDR_APP_IMAGE || addr_flash >= ADDR_APP_IMAGE + LEN_APP_IMAGE)
                        {
                            tx_buf[2] = SSP_FLASH_WRONG_ADDRESS;
                        }
                        else
                        {
                            tx_buf[2] = SSP_FLASH_OK;
                            if(addr_flash == ADDR_APP_IMAGE)
                            {
                                len_flash = 4;
                                if(data_len != 6 + 4 /*data*/ + 4 /* len */ + 4 /*crc*/)
                                {
                                    tx_buf[2] = SSP_FLASH_WRONG_CRC_ADDRESS;
                                }
                                else
                                {
                                    uint32_t first_word, len_full, crc_ref;
                                    memcpy_volatile(&first_word, data + 6, 4);
                                    memcpy_volatile(&len_full, data + 6 + 4, 4);
                                    memcpy_volatile(&crc_ref, data + 6 + 4 + 4, 4);

                                    if(len_full >= LEN_APP_IMAGE || (len_full % 4) != 0)
                                    {
                                        tx_buf[2] = SSP_FLASH_WRONG_FULL_LENGTH;
                                    }
                                    else
                                    {
                                        CRC->CR |= CRC_CR_RESET;
                                        CRC->DR = first_word;
                                        for(uint32_t i = ADDR_APP_IMAGE + 4; i < (ADDR_APP_IMAGE + len_full); i += 4)
                                        {
                                            CRC->DR = *(uint32_t *)i;
                                        }
                                        if(CRC->DR != crc_ref)
                                        {
                                            tx_buf[2] = SSP_FLASH_WRONG_CRC;
                                        }
                                    }
                                }
                            }
                            if(tx_buf[2] == SSP_FLASH_OK)
                            {
                                for(uint32_t i = 0; i < len_flash; i++)
                                {
                                    if(FLASH_ProgramByte(addr_flash + i, data[6 + i]) != FLASH_COMPLETE)
                                    {
                                        tx_buf[2] = SSP_FLASH_FAIL;
                                        break;
                                    }
                                    if(*(uint8_t *)(addr_flash + i) != data[6 + i])
                                    {
                                        tx_buf[2] = SSP_FLASH_VERIFY_FAIL;
                                        break;
                                    }
                                }
                            }

                            memcpy(tx_buf + 3, &addr_flash, 4);
                            air_protocol_send_async(iterator_ctrl, tx_buf, 3 + 4);
                            break;
                        }

                        air_protocol_send_async(iterator_ctrl, tx_buf, 3);
                    }
                    break;

                    default:
                    {
                        tx_buf[2] = SSP_FLASH_UNEXIST;
                        air_protocol_send_async(iterator_ctrl, tx_buf, 3);
                    }
                    break;
                    }
                }
                else
                {
                    tx_buf[2] = SSP_FLASH_WRONG_PARAM;
                    tx_buf[3] = data_len;
                    air_protocol_send_async(iterator_ctrl, tx_buf, 4);
                }
            }
            else
            {
                tx_buf[2] = SSP_FLASH_WRONG_PARAM;
                tx_buf[3] = data_len;
                air_protocol_send_async(iterator_ctrl, tx_buf, 4);
            }
        }
        break;

        default:
            // debug("Unknown cmd %d\n", data[0]);
            break;
        }
    }
}