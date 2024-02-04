#include "air_protocol.h"
#include "debug.h"
#include "flash_regions.h"
#include "flasher_hal.h"
#include "main.h"
#include "power.h"
#include "serial_suit_protocol.h"
#include "usbd_cdc_if.h"

#include <stdbool.h>
#include <stdint.h>

extern const uint8_t iterator_head;
extern const uint8_t iterator_tail;

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t tx_buf[1024];

static uint32_t crc32_calc(const uint32_t *buf, uint32_t len)
{
    CRC->CR |= CRC_CR_RESET;

    for(uint32_t i = 0; i < len; i++)
    {
        CRC->DR = buf[i];
    }

    return CRC->DR;
}

static void tx(uint8_t *buf, uint32_t len)
{
    CDC_Transmit_FS(buf, len);
}

void usb_if_rx(uint8_t *buf, uint32_t len)
{
    if(len)
    {
        switch(buf[0])
        {
        case RFM_NET_CMD_DEBUG:
            if(len >= 3) // at least 1 symbol
            {
                switch(buf[1])
                {
                case RFM_NET_ID_CTRL:
                    for(uint32_t i = 2; i < len; i++)
                        debug_rx(buf[i]);
                    break;

                case RFM_NET_ID_HEAD:
                    air_protocol_send_async(iterator_head, buf, len);
                    break;

                case RFM_NET_ID_TAIL:
                    air_protocol_send_async(iterator_tail, buf, len);
                    break;

                default:
                    break;
                }
            }
            break;

        case RFM_NET_CMD_REBOOT:
            if(len >= 2)
            {
                switch(buf[1])
                {
                case RFM_NET_ID_CTRL:
                {
                    // USB_DevDisconnect(hpcd_USB_OTG_FS.Instance);
                    // HAL_Delay(100);
                    HAL_NVIC_SystemReset();
                }
                break;

                case RFM_NET_ID_HEAD:
                    air_protocol_send_async(iterator_head, buf, len);
                    break;

                case RFM_NET_ID_TAIL:
                    air_protocol_send_async(iterator_tail, buf, len);
                    break;

                default:
                    break;
                }
            }
            break;

        case RFM_NET_CMD_OFF:
         if(len >= 2)
            {
                switch(buf[1])
                {
                case RFM_NET_ID_CTRL:
                    pwr_sleep();
                    break;

                case RFM_NET_ID_HEAD:
                    air_protocol_send_async(iterator_head, buf, len);
                    break;

                case RFM_NET_ID_TAIL:
                    air_protocol_send_async(iterator_tail, buf, len);
                    break;

                default:
                    break;
                }
            }
            break;

        case RFM_NET_CMD_FLASH:
            // Format: [1b] RFM_NET_CMD_FLASH [1b] node_id [4b] address [xx] data
            if(len >= 7) // at least 1 symbol
            {
                if(((len - 6) % 4) == 0)
                {
                    tx_buf[0] = RFM_NET_CMD_FLASH;
                    tx_buf[1] = buf[1];

                    uint32_t addr, len_flash = len - 6;
                    memcpy(&addr, buf + 2, 4);

                    switch(buf[1])
                    {
                    case RFM_NET_ID_CTRL:
                    {
                        debug_disable_usb();
                        if(addr < ADDR_APP_IMAGE || addr >= ADDR_APP_IMAGE + LEN_APP_IMAGE)
                        {
                            tx_buf[2] = SSP_FLASH_WRONG_ADDRESS;
                        }
                        else
                        {
                            tx_buf[2] = SSP_FLASH_OK;
                            if(addr == ADDR_APP_IMAGE)
                            {
                                len_flash = 4;
                                if(len != 6 + 4 /*data*/ + 4 /* len */ + 4 /*crc*/)
                                {
                                    tx_buf[2] = SSP_FLASH_WRONG_CRC_ADDRESS;
                                }
                                else
                                {
                                    uint32_t first_word, len_full, crc_ref;
                                    memcpy(&first_word, buf + 6, 4);
                                    memcpy(&len_full, buf + 6 + 4, 4);
                                    memcpy(&crc_ref, buf + 6 + 4 + 4, 4);

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
                                    if(FLASH_ProgramByte(addr + i, buf[6 + i]) != FLASH_COMPLETE)
                                    {
                                        tx_buf[2] = SSP_FLASH_FAIL;
                                        break;
                                    }
                                    if(*(uint8_t *)(addr + i) != buf[6 + i])
                                    {
                                        tx_buf[2] = SSP_FLASH_VERIFY_FAIL;
                                        break;
                                    }
                                }
                            }
                            memcpy(tx_buf + 3, &addr, 4);
                            tx(tx_buf, 3 + 4);
                            break;
                        }

                        tx(tx_buf, 3);
                    }
                    break;

                    case RFM_NET_ID_HEAD:
                    {
                        air_protocol_set_periodic_sleep(false);
                        // tx_buf[2] = SSP_FLASH_UNEXIST;
                        // tx(tx_buf, 3);
                        air_protocol_send_async(iterator_head, buf, len);
                    }
                    break;

                    case RFM_NET_ID_TAIL:
                    {
                        air_protocol_set_periodic_sleep(false);
                        // tx_buf[2] = SSP_FLASH_UNEXIST;
                        // tx(tx_buf, 3);
                        air_protocol_send_async(iterator_tail, buf, len);
                    }
                    break;

                    default:
                    {
                        tx_buf[2] = SSP_FLASH_UNEXIST;
                        tx(tx_buf, 3);
                    }
                    break;
                    }
                }
                else
                {
                    tx_buf[2] = SSP_FLASH_WRONG_PARAM;
                    tx_buf[3] = len;
                    tx(tx_buf, 4);
                }
            }
            break;

        case RFM_NET_CMD_FLASH_STOP:
            air_protocol_set_periodic_sleep(true);
            break;

        default:
            break;
        }
    }
}