#ifndef FLASH_H
#define FLASH_H

#include "stm32f4xx_hal.h"

typedef enum
{
    FLASH_BUSY = 1,
    FLASH_ERROR_PROGRAM = 7,
    FLASH_COMPLETE = 9
} FLASH_Status;

#define VoltageRange_1 ((uint8_t)0x00) /*!< Device operating range: 1.8V to 2.1V */
#define VoltageRange_2 ((uint8_t)0x01) /*!<Device operating range: 2.1V to 2.7V */
#define VoltageRange_3 ((uint8_t)0x02) /*!<Device operating range: 2.7V to 3.6V */
#define VoltageRange_4 ((uint8_t)0x03) /*!<Device operating range: 2.7V to 3.6V + External Vpp */

#define FLASH_Sector_0 ((uint16_t)0x0000)  /*!< Sector Number 0   */
#define FLASH_Sector_1 ((uint16_t)0x0008)  /*!< Sector Number 1   */
#define FLASH_Sector_2 ((uint16_t)0x0010)  /*!< Sector Number 2   */
#define FLASH_Sector_3 ((uint16_t)0x0018)  /*!< Sector Number 3   */
#define FLASH_Sector_4 ((uint16_t)0x0020)  /*!< Sector Number 4   */
#define FLASH_Sector_5 ((uint16_t)0x0028)  /*!< Sector Number 5   */
#define FLASH_Sector_6 ((uint16_t)0x0030)  /*!< Sector Number 6   */
#define FLASH_Sector_7 ((uint16_t)0x0038)  /*!< Sector Number 7   */
#define FLASH_Sector_8 ((uint16_t)0x0040)  /*!< Sector Number 8   */
#define FLASH_Sector_9 ((uint16_t)0x0048)  /*!< Sector Number 9   */
#define FLASH_Sector_10 ((uint16_t)0x0050) /*!< Sector Number 10  */
#define FLASH_Sector_11 ((uint16_t)0x0058) /*!< Sector Number 11  */
#define FLASH_Sector_12 ((uint16_t)0x0080) /*!< Sector Number 12  */
#define FLASH_Sector_13 ((uint16_t)0x0088) /*!< Sector Number 13  */
#define FLASH_Sector_14 ((uint16_t)0x0090) /*!< Sector Number 14  */
#define FLASH_Sector_15 ((uint16_t)0x0098) /*!< Sector Number 15  */
#define FLASH_Sector_16 ((uint16_t)0x00A0) /*!< Sector Number 16  */
#define FLASH_Sector_17 ((uint16_t)0x00A8) /*!< Sector Number 17  */
#define FLASH_Sector_18 ((uint16_t)0x00B0) /*!< Sector Number 18  */
#define FLASH_Sector_19 ((uint16_t)0x00B8) /*!< Sector Number 19  */
#define FLASH_Sector_20 ((uint16_t)0x00C0) /*!< Sector Number 20  */
#define FLASH_Sector_21 ((uint16_t)0x00C8) /*!< Sector Number 21  */
#define FLASH_Sector_22 ((uint16_t)0x00D0) /*!< Sector Number 22  */
#define FLASH_Sector_23 ((uint16_t)0x00D8) /*!< Sector Number 23  */

void FLASH_Unlock(void);
void FLASH_Lock(void);

void FLASH_ClearFlag(uint32_t FLASH_FLAG);

FLASH_Status FLASH_GetStatus(void);
FLASH_Status _FLASH_WaitForLastOperation(void);

FLASH_Status FLASH_EraseSector(uint32_t FLASH_Sector, uint8_t VoltageRange);
FLASH_Status FLASH_ProgramByte(uint32_t Address, uint8_t Data);

#endif // FLASH_H