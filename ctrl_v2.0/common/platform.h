#ifndef PLATFORM_H
#define PLATFORM_H

#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>

#define _BV(x) (1ULL << (x))

#define FLASH_LEN (0x00040000U) // 256kB
#define FLASH_START FLASH_BASE
#define FLASH_ORIGIN FLASH_BASE
#define FLASH_FINISH (FLASH_BASE + FLASH_LEN)
#define FLASH_SIZE FLASH_LEN

#define UNIQUE_ID 0x1FFF7A10

#define BUTTONS_POLL()                                               \
	debounce_update(&btn[0][0], !(GPIOC->IDR & (1 << 14)), diff_ms); \
	debounce_update(&btn[0][1], !(GPIOC->IDR & (1 << 11)), diff_ms); \
	debounce_update(&btn[0][2], !(GPIOC->IDR & (1 << 8)), diff_ms);  \
                                                                     \
	debounce_update(&btn[1][0], !(GPIOC->IDR & (1 << 15)), diff_ms); \
	debounce_update(&btn[1][1], !(GPIOA->IDR & (1 << 15)), diff_ms); \
	debounce_update(&btn[1][2], !(GPIOC->IDR & (1 << 7)), diff_ms);  \
                                                                     \
	debounce_update(&btn[2][0], !(GPIOA->IDR & (1 << 4)), diff_ms);  \
	debounce_update(&btn[2][1], !(GPIOC->IDR & (1 << 10)), diff_ms); \
	debounce_update(&btn[2][2], !(GPIOB->IDR & (1 << 15)), diff_ms); \
                                                                     \
	debounce_update(&btn[3][0], !(GPIOA->IDR & (1 << 5)), diff_ms);  \
	debounce_update(&btn[3][1], !(GPIOA->IDR & (1 << 3)), diff_ms);  \
	debounce_update(&btn[3][2], !(GPIOB->IDR & (1 << 12)), diff_ms);

void platform_flash_erase_flag_reset(void);
void platform_flash_erase_flag_reset_sect_cfg(void);

void platform_flash_unlock(void);
void platform_flash_lock(void);
int platform_flash_read(uint32_t addr, uint8_t *src, uint32_t sz);
int platform_flash_write(uint32_t dest, const uint8_t *src, uint32_t sz);

void platform_init(void);
void platform_deinit(void);
void platform_reset(void);
void platform_reset_jump_ldr_app(void);
void platform_run_address(uint32_t address);

void platform_get_uid(uint32_t *id);

void delay_ms(volatile uint32_t delay_ms);

void platform_watchdog_init(void);
static inline void platform_watchdog_reset(void) { IWDG_ReloadCounter(); }

const char *platform_reset_cause_get(void);

void main_poll(void);
void pwr_off(void);

void _lseek_r(void);
void _close_r(void);
void _read_r(void);
void _write_r(void);

extern int __preldr_start, __preldr_end;
extern int __ldr_start, __ldr_end;
extern int __cfg_start, __cfg_end;
extern int __app_start, __app_end;
extern int __header_offset;

extern uint32_t g_uid[3];
extern volatile uint32_t system_time_ms;

#endif // PLATFORM_H
