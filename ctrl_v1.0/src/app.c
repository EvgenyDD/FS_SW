#include "adc.h"
#include "air_protocol.h"
#include "debounce.h"
#include "debug.h"
#include "flasher_hal.h"
#include "hb_tracker.h"
#include "helper.h"
#include "main.h"
#include "power.h"
#include "rfm12b.h"
#include "usbd_cdc_if.h"

#include <string.h>

#define CYCLIC_OP(num, op)                \
    {                                     \
        for(uint32_t i = 0; i < num; i++) \
            op;                           \
    }

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;
extern USBD_HandleTypeDef hUsbDeviceFS;

hbt_node_t *hbt_head;

bool to_from_head = true;

bool g_charge_en = true;

uint32_t get_random(void) { return HAL_RNG_GetRandomNumber(&hrng); }

const uint8_t iterator_head = 0;
const uint8_t iterator_tail = 1;

static button_ctrl_t btn[4][3];

static bool btn_pwr_off_long_press = false;

void init(void)
{
    for(uint32_t i = 0; i < 4; i++)
        for(uint32_t k = 0; k < 3; k++)
            debounce_init(&btn[i][k], 250, 800);

    debug_init();
    adc_init();

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    hbt_head = hb_tracker_init(RFM_NET_ID_HEAD, 700 /* 2x HB + 100 ms*/);

    air_protocol_init();
    air_protocol_init_node(iterator_head, RFM_NET_ID_HEAD);
    air_protocol_init_node(iterator_tail, RFM_NET_ID_TAIL);
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
        LED0_GPIO_Port->ODR ^= LED0_Pin;
        // LED1_GPIO_Port->ODR ^= LED1_Pin;
        // LED2_GPIO_Port->ODR ^= LED2_Pin;
        // LED3_GPIO_Port->ODR ^= LED3_Pin;
        // LED4_GPIO_Port->ODR ^= LED4_Pin;
        // LED5_GPIO_Port->ODR ^= LED5_Pin;

        static uint32_t cnt = 0;
        // debug(">>> %d\n", cnt++);
        if(0)
        {
            static uint8_t sendSize = 0;
            static bool request_ack = false;

            debug("Sending[%d]\n", sendSize + 1);
            // for(uint32_t i = 0; i < sendSize + 1U; i++)
            //     debug(" > %c\n", (char)payload[i]);

            request_ack = !(sendSize % 3); //request ACK every 3rd xmission
        }
    }

    if(btn_pwr_off_long_press) LED0_GPIO_Port->ODR |= LED0_Pin;

    // btn
    {
        debounce_cb(&btn[0][0], B0_WKUP_GPIO_Port->IDR & B0_WKUP_Pin, diff_ms);
        debounce_cb(&btn[0][1], !(B1_GPIO_Port->IDR & B1_Pin), diff_ms);
        debounce_cb(&btn[0][2], !(B2_GPIO_Port->IDR & B2_Pin), diff_ms);

        debounce_cb(&btn[1][0], !(B3_GPIO_Port->IDR & B3_Pin), diff_ms);
        debounce_cb(&btn[1][1], !(B4_GPIO_Port->IDR & B4_Pin), diff_ms);
        debounce_cb(&btn[1][2], !(B5_GPIO_Port->IDR & B5_Pin), diff_ms);

        debounce_cb(&btn[2][0], !(B6_GPIO_Port->IDR & B6_Pin), diff_ms);
        debounce_cb(&btn[2][1], !(B7_GPIO_Port->IDR & B7_Pin), diff_ms);
        debounce_cb(&btn[2][2], !(B8_GPIO_Port->IDR & B8_Pin), diff_ms);

        debounce_cb(&btn[3][0], !(B9_GPIO_Port->IDR & B9_Pin), diff_ms);
        debounce_cb(&btn[3][1], !(B10_GPIO_Port->IDR & B10_Pin), diff_ms);
        debounce_cb(&btn[3][2], !(B11_GPIO_Port->IDR & B11_Pin), diff_ms);
    }

    // self-off control
    static uint32_t to_without_press = 0;
    to_without_press += diff_ms;
    for(uint32_t i = 0; i < 4; i++)
        for(uint32_t k = 0; k < 3; k++)
            if(btn[i][k].pressed) to_without_press = 0;
    if(hb_tracker_is_timeout(hbt_head) == false) to_without_press = 0;
    // if(pwr_is_charging()) to_without_press = 0; // don't work ok
    if(hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) to_without_press = 0;

    if(to_without_press > 15000 ||
       adc_drv_get_vbat() < 3.2f) pwr_sleep();

    // off button
    {
        if(btn[0][0].pressed_shot_long) btn_pwr_off_long_press = true;
        if(btn[0][0].unpressed_shot && btn_pwr_off_long_press) pwr_sleep();
    }

    for(uint32_t i = 0; i < 4; i++)
        for(uint32_t k = 0; k < 3; k++)
            if(btn[i][k].pressed_shot) debug(DBG_INFO "Btn %d %d pressed\n", i, k);
    for(uint32_t i = 0; i < 4; i++)
        for(uint32_t k = 0; k < 3; k++)
            if(btn[i][k].pressed_shot_long) debug(DBG_INFO "Btn %d %d pressed long\n", i, k);
    for(uint32_t i = 0; i < 4; i++)
        for(uint32_t k = 0; k < 3; k++)
            if(btn[i][k].unpressed_shot) debug(DBG_INFO "Btn %d %d unpressed\n", i, k);

    air_protocol_poll();

    static uint32_t prev_ticke = 0;

    if(prev_ticke < HAL_GetTick())
    {
        prev_ticke = HAL_GetTick() + 100;

        if(hb_tracker_is_timeout(hbt_head))
        {
            LED7_GPIO_Port->ODR |= LED7_Pin;
        }
        else
        {
            LED7_GPIO_Port->ODR ^= LED7_Pin;
        }
    }

    if(hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
    {
        LED4_GPIO_Port->ODR |= LED4_Pin;
    }
    else
    {
        LED4_GPIO_Port->ODR &= (uint32_t)~LED4_Pin;
    }

    static uint32_t ctrl_hb = 0;
    if(ctrl_hb < HAL_GetTick())
    {
        ctrl_hb = HAL_GetTick() + 800;

        uint8_t r = 0, g = 200, b = 0;
        uint8_t data[4] = {RFM_NET_CMD_LIGHT, 0, 0, 0};
        static uint8_t ptr = 1;
        ptr++;
        if(ptr >= 4) ptr = 1;
        data[ptr] = 200;
        // air_protocol_send_async(iterator_head, data, sizeof(data));

        // static uint32_t fail_cnt = 0;
        // if(sts) fail_cnt++;
        // if(sts)
        //     debug("Light %d fail %d\n", sts ? 1 : 0, fail_cnt);
    }

    static uint32_t prev = 0;
    static uint32_t cnt = 0;
    if(prev < HAL_GetTick())
    {
        prev = HAL_GetTick() + 8000;
        CHRG_EN_GPIO_Port->ODR &= (uint32_t)~CHRG_EN_Pin;
// HAL_Delay(100);
#warning "HERE^^^"
        debug(DBG_INFO "STAT: %d | %.3fV\n", cnt >> 3, adc_drv_get_vbat());
        if(g_charge_en)
            CHRG_EN_GPIO_Port->ODR |= CHRG_EN_Pin;
        cnt = 0;
    }
    cnt++;

    static int ear_move_step = -1;
    static uint32_t ear_move_delay = 0;

    if(ear_move_step != -1 && ear_move_delay < HAL_GetTick())
    {
        static uint8_t data_ear_smooth_servo[] = {RFM_NET_CMD_SERVO_SMOOTH, 0, 0,
                                                  2 /*0*/, 0, 0,
                                                  3 /*1*/, 0, 0};
        uint16_t time = 300;
        memcpy(&data_ear_smooth_servo[1], &time, 2);
        uint16_t pos[2][2] = {
            {750, 300},
            {350, 750},
        };

        for(uint32_t i = 0; i < 2; i++)
            memcpy(&data_ear_smooth_servo[3 + (3 * i + 1)], &pos[ear_move_step % 2][i], 2);
        air_protocol_send_async(iterator_head, data_ear_smooth_servo, sizeof(data_ear_smooth_servo));

        if(++ear_move_step >= 4) ear_move_step = -1;
        ear_move_delay = HAL_GetTick() + 400;
    }

    // button handler
    {
        if(btn[0][0].pressed_shot)
        {
            // disable all servos
            uint8_t data[] = {RFM_NET_CMD_SERVO_RAW, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
        }

        if(btn[1][0].pressed_shot)
        {
            ear_move_step = 0;
            ear_move_delay = 0;
        }

        if(btn[0][1].pressed_shot)
        {
            static const uint8_t fan_lo[] = {0, 60, 80, 95};
            static const uint8_t fan_hi[] = {0, 0, 70, 95};
            static uint32_t mode = 0;
            mode++;
            if(mode >= sizeof(fan_lo) / sizeof(fan_lo[0])) mode = 0;
            uint8_t data[] = {RFM_NET_CMD_FAN, 0, 0};
            data[1] = fan_hi[mode];
            data[2] = fan_lo[mode];
            CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
        }

        if(btn[1][1].pressed_shot)
        {
            static const uint8_t leds[3][4] = {
                {
                    0,
                    120,
                    0,
                    0,
                },
                {
                    0,
                    0,
                    120,
                    0,
                },
                {
                    0,
                    0,
                    0,
                    120,
                }};
            static uint32_t mode = 0;
            mode++;
            if(mode >= 4) mode = 0;
            uint8_t data[] = {RFM_NET_CMD_LIGHT, 0, 0, 0};
            data[1] = leds[0][mode];
            data[2] = leds[1][mode];
            data[3] = leds[2][mode];
            CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
        }

        if(btn[2][1].pressed_shot)
        {
            static const uint16_t servo_tongue[] = {320, /*470,*/ 620};
            static uint32_t mode = 0;
            mode++;
            if(mode >= sizeof(servo_tongue) / sizeof(servo_tongue[0])) mode = 0;
            uint8_t data[] = {RFM_NET_CMD_SERVO_RAW, 6, 0, 0};
            memcpy(&data[2], &servo_tongue[mode], 2);
            CYCLIC_OP(3, air_protocol_send_async(iterator_head, data, sizeof(data)));
        }

        static uint8_t data_smooth_servo[] = {RFM_NET_CMD_SERVO_SMOOTH, 0, 0,
                                              0 /*0*/, 0, 0,
                                              1 /*1*/, 0, 0,
                                              4 /*4*/, 0, 0,
                                              5 /*5*/, 0, 0};
        if(btn[0][2].pressed_shot)
        {
            uint16_t time = 800;
            memcpy(&data_smooth_servo[1], &time, 2);
            uint16_t pos[4] = {500, 560, 370, 330};
            for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
                memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
            air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
        }

        if(btn[1][2].pressed_shot)
        {
            uint16_t time = 800;
            memcpy(&data_smooth_servo[1], &time, 2);
            uint16_t pos[4] = {340, 380, 600, 500};
            for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
                memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
            air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
        }

        if(btn[2][2].pressed_shot)
        {
            uint16_t pos[4] = {500, 420, 550, 330};
            for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
                memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
            air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
        }

        if(btn[3][2].pressed_shot)
        {
            // uint16_t pos[4] = {440,330,640,400}; // alt
            uint16_t pos[4] = {500, 330, 620, 330};
            for(uint32_t i = 0; i < sizeof(pos) / sizeof(pos[0]); i++)
                memcpy(&data_smooth_servo[3 + (3 * i + 1)], &pos[i], 2);
            air_protocol_send_async(iterator_head, data_smooth_servo, sizeof(data_smooth_servo));
        }
    }
}

inline static void memcpy_volatile(void *dst, const volatile void *src, size_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        *((uint8_t *)dst + i) = *((const volatile uint8_t *)src + i);
    }
}

static uint8_t dbg_buffer[RFM12B_MAXDATA];
void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
    // debug("RX: %d > Size: %d | %d %d %d %d\n", sender_node_id, data_len, *(data-4), *(data-3),*(data-2),*(data-1));
    // debug(DBG_INFO"R %d > %d\n", sender_node_id, data[0]);
    if(data_len > 0)
    {
        switch(data[0])
        {
        case RFM_NET_CMD_DEBUG:
            memcpy_volatile(dbg_buffer, data + 1, (size_t)(data_len - 1));
            if(dbg_buffer[data_len - 2] != '\n')
            {
                dbg_buffer[data_len - 1] = '\n';
                dbg_buffer[data_len - 0] = '\0';
            }
            else
            {
                dbg_buffer[data_len - 1] = '\0';
            }
            debug("[***]\t%s\n", dbg_buffer);
            // for(uint8_t i = 1; i < data_len; i++)
            // debug("%c", data[i]);
            // if(data[data_len - 1] != '\n') debug("\n");
            break;

            // case RFM_NET_CMD_HB:
            // {
            //     if(data_len == 2)
            //     {
            //         bool not_found = hb_tracker_update(sender_node_id);
            //         if(not_found) debug("HB unknown %d\n", sender_node_id);
            //         if(sender_node_id == RFM_NET_ID_HEAD)
            //         {
            //             to_from_head = data[1];
            //         }
            //     }
            // }
            // break;

        case RFM_NET_CMD_STS_HEAD:
            if(data_len == 1 + 4 + 4)
            {
                float vbat, temp;
                memcpy_volatile(&vbat, (data + 1), 4);
                memcpy_volatile(&temp, (data + 1 + 4), 4);
                debug(DBG_INFO "HD STS: %.2fV (%.1f%%) | t %.1f\n",
                      vbat,
                      map_float(vbat, 3.1f * 2.f, 4.18f * 2.f, 0, 100.f),
                      temp);
            }
            break;

        case RFM_NET_CMD_FLASH:
        {
            static uint8_t temp_data[CDC_DATA_FS_OUT_PACKET_SIZE];
            uint32_t sz = data_len < CDC_DATA_FS_OUT_PACKET_SIZE ? data_len : CDC_DATA_FS_OUT_PACKET_SIZE;
            for(uint32_t i = 0; i < sz; i++)
            {
                temp_data[i] = data[i];
            }
            CDC_Transmit_FS(temp_data, sz);
        }
        break;

        default:
            debug(DBG_WARN "Unknown cmd %d\n", data[0]);
            break;
        }
    }
}