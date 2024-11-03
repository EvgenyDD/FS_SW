#include "flash.h"
#include "flash_regions.h"
#include "main.h"

#include <stdbool.h>
#include <string.h>

bool app_image_present = false, app_present = false, app_image_not_empty = false;

extern CRC_HandleTypeDef hcrc;

static volatile uint32_t time_up_ms = 0;
static volatile uint32_t flash_counter = 0;

static volatile uint32_t period = 90;

static volatile bool flash_fast = true;

void systick_callback(void)
{
    time_up_ms++;
    flash_counter++;

    if(flash_fast)
    {
        static uint32_t prev_tick = 0;
        prev_tick++;
        if((prev_tick % period) < 5)
        {
            LED_GPIO_Port->ODR |= LED_Pin;
        }
        else
        {
            LED_GPIO_Port->ODR &= (uint32_t) ~(LED_Pin);
        }
    }
}

static uint32_t seek_flash_start(uint32_t start, uint32_t end)
{
    uint8_t *_end = (uint8_t *)end;
    uint8_t *_start = (uint8_t *)start;
    for(; _end > _start; _end--)
    {
        if(*_end != 0xFF) return (uint32_t)_end;
    }

    return start;
}

uint32_t app_image_end;
uint32_t app_end;

uint32_t countdown_turn_off = 10000;

static void goto_app(void)
{
    __disable_irq();
    LED_GPIO_Port->ODR |= LED_Pin;

    typedef void (*app_entry_t)(void);
    app_entry_t app = (app_entry_t)(*(uint32_t *)(ADDR_APP + 4));

    __set_MSP(*(uint32_t *)(ADDR_APP));
    SCB->VTOR = ADDR_APP;
    app();
}

static bool compare(void)
{
    for(uint32_t i = ADDR_APP, j = ADDR_APP_IMAGE; i <= app_end || j < app_image_end; i++, j++)
    {
        if(*(uint8_t *)i != *(uint8_t *)j) return false;
    }
    return true;
}

static void copy_image(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    FLASH_EraseSector(FLASH_Sector_5, VoltageRange_3);
    if(app_image_end - ADDR_APP_IMAGE > 0x20000U)
        FLASH_EraseSector(FLASH_Sector_6, VoltageRange_3);

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    const uint8_t *data = (uint8_t *)ADDR_APP_IMAGE;
    for(uint32_t i = 0; i <= app_image_end - ADDR_APP_IMAGE; i++)
    {
        FLASH_ProgramByte(ADDR_APP + i, data[i]);
    }
}

static void flash_led(uint32_t count, uint32_t period_flash)
{
    flash_fast = false;

    for(flash_counter = 0; flash_counter < count * period_flash * 2;)
    {
        if((flash_counter % (2 * period_flash)) == 0)
        {
            LED_GPIO_Port->ODR |= LED_Pin;
        }
        else if((flash_counter % period_flash) == 0)
        {
            LED_GPIO_Port->ODR &= (uint32_t) ~(LED_Pin);
        }
        else
        {
            asm("nop");
        }
    }

    flash_fast = true;
}

static void erase_app_image(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
    if(app_image_end - ADDR_APP_IMAGE > 0x20000U)
        FLASH_EraseSector(FLASH_Sector_8, VoltageRange_3);

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

void init(void)
{
    app_end = seek_flash_start(ADDR_APP, ADDR_APP + LEN_APP - 1);
    app_present = app_end != ADDR_APP;

    app_image_end = seek_flash_start(ADDR_APP_IMAGE, ADDR_APP_IMAGE + LEN_APP_IMAGE - 1);
    app_image_not_empty = app_image_end != ADDR_APP_IMAGE;
    app_image_present = app_image_not_empty && *(uint32_t *)ADDR_APP_IMAGE != 0xFFFFFFFF;

    if(app_present == false && app_image_not_empty == false)
    {
        countdown_turn_off = 2000;
    }
    else if(app_present && app_image_not_empty == false)
    {
        if(*(uint32_t *)ADDR_APP_IMAGE != 0xFFFFFFFF)
        {
            erase_app_image();
        }
        goto_app();
    }
    else if(app_present && app_image_not_empty)
    {
        bool equal = compare();
        if(equal == false && app_image_present)
        {
            copy_image();
            flash_led(1, 250);
        }
        {
            erase_app_image();
            flash_led(4, 250);
        }
        goto_app();
    }
    else if(app_present == false && app_image_not_empty)
    {
        if(app_image_present)
        {
            copy_image();
        }
        erase_app_image();
        flash_led(3, 250);
        goto_app();
    }
    flash_fast = true;
}

void loop(void)
{
    if(time_up_ms > countdown_turn_off)
    {
        flash_led(2, 250);
        PWR_EN_GPIO_Port->ODR &= (uint32_t) ~(PWR_EN_Pin);
        period = 200;
    }
}
