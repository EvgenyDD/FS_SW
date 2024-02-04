#include "stm32f4xx_hal.h"
#include "flash.h"

#define SECTOR_MASK ((uint32_t)0xFFFFFF07)
#define FLASH_FLAG_RDERR ((uint32_t)0x00000100) /*!< Read Protection error flag (PCROP)        */

void FLASH_Unlock(void)
{
    if((FLASH->CR & FLASH_CR_LOCK) != RESET)
    {
        /* Authorize the FLASH Registers access */
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}


void FLASH_Lock(void) { FLASH->CR |= FLASH_CR_LOCK; }

void FLASH_ClearFlag(uint32_t FLASH_FLAG) { FLASH->SR = FLASH_FLAG; }

FLASH_Status FLASH_GetStatus(void)
{
    FLASH_Status flashstatus = FLASH_COMPLETE;

    if((FLASH->SR & FLASH_FLAG_BSY) == FLASH_FLAG_BSY)
    {
        flashstatus = FLASH_BUSY;
    }
    else
    {
        if((FLASH->SR & FLASH_FLAG_WRPERR) != (uint32_t)0x00)
        {
            flashstatus = FLASH_ERROR_WRP;
        }
        else
        {
            if((FLASH->SR & FLASH_FLAG_RDERR) != (uint32_t)0x00)
            {
                flashstatus = FLASH_ERROR_RD;
            }
            else
            {
                if((FLASH->SR & (uint32_t)0xE0) != (uint32_t)0x00)
                {
                    flashstatus = FLASH_ERROR_PROGRAM;
                }
                else
                {
                    if((FLASH->SR & FLASH_FLAG_OPERR) != (uint32_t)0x00)
                    {
                        flashstatus = FLASH_ERROR_OPERATION;
                    }
                    else
                    {
                        flashstatus = FLASH_COMPLETE;
                    }
                }
            }
        }
    }
    /* Return the FLASH Status */
    return flashstatus;
}

FLASH_Status _FLASH_WaitForLastOperation(void)
{ 
  __IO FLASH_Status status = FLASH_COMPLETE;
   
  /* Check for the FLASH Status */
  status = FLASH_GetStatus();

  /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
     Even if the FLASH operation fails, the BUSY flag will be reset and an error
     flag will be set */
  while(status == FLASH_BUSY)
  {
    status = FLASH_GetStatus();
  }
  /* Return the operation status */
  return status;
}

FLASH_Status FLASH_EraseSector(uint32_t FLASH_Sector, uint8_t VoltageRange)
{
    uint32_t tmp_psize = 0x0;

    if(VoltageRange == VoltageRange_1)
    {
        tmp_psize = FLASH_PSIZE_BYTE;
    }
    else if(VoltageRange == VoltageRange_2)
    {
        tmp_psize = FLASH_PSIZE_HALF_WORD;
    }
    else if(VoltageRange == VoltageRange_3)
    {
        tmp_psize = FLASH_PSIZE_WORD;
    }
    else
    {
        tmp_psize = FLASH_PSIZE_DOUBLE_WORD;
    }
    /* Wait for last operation to be completed */
    FLASH_Status status = _FLASH_WaitForLastOperation();

    if(status == FLASH_COMPLETE)
    {
        /* if the previous operation is completed, proceed to erase the sector */
        FLASH->CR &= CR_PSIZE_MASK;
        FLASH->CR |= tmp_psize;
        FLASH->CR &= SECTOR_MASK;
        FLASH->CR |= FLASH_CR_SER | FLASH_Sector;
        FLASH->CR |= FLASH_CR_STRT;

        status = _FLASH_WaitForLastOperation();

        /* if the erase operation is completed, disable the SER Bit */
        FLASH->CR &= (~FLASH_CR_SER);
        FLASH->CR &= SECTOR_MASK;
    }

    return status;
}

FLASH_Status FLASH_ProgramByte(uint32_t Address, uint8_t Data)
{
    /* Wait for last operation to be completed */
    FLASH_Status status = _FLASH_WaitForLastOperation();

    if(status == FLASH_COMPLETE)
    {
        /* if the previous operation is completed, proceed to program the new data */
        FLASH->CR &= CR_PSIZE_MASK;
        FLASH->CR |= FLASH_PSIZE_BYTE;
        FLASH->CR |= FLASH_CR_PG;

        *(__IO uint8_t *)Address = Data;

        /* Wait for last operation to be completed */
        status = _FLASH_WaitForLastOperation();

        /* if the program operation is completed, disable the PG Bit */
        FLASH->CR &= (~FLASH_CR_PG);
    }

    /* Return the Program Status */
    return status;
}
