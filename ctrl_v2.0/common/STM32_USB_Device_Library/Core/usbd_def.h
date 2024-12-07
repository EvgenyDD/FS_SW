/**
  ******************************************************************************
  * @file    usbd_def.h
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    17-March-2018
  * @brief   general defines for the usb device library 
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      <http://www.st.com/SLA0044>
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __USBD_DEF_H
#define __USBD_DEF_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_conf.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */
  
/** @defgroup USB_DEF
  * @brief general defines for the usb device library file
  * @{
  */ 

/** @defgroup USB_DEF_Exported_Defines
  * @{
  */ 

#ifndef NULL
#define NULL    0
#endif

#define USB_LEN_DEV_QUALIFIER_DESC                               0x0A
#define USB_LEN_DEV_DESC                                         0x12
#define USB_LEN_CFG_DESC                                         0x09
#define USB_LEN_IF_DESC                                          0x09
#define USB_LEN_EP_DESC                                          0x07
#define USB_LEN_OTG_DESC                                         0x03
#define	USB_LEN_OS_DESC                                          0x12
#define USB_LEN_LANGID_DESC                                      0x04
#define USB_LEN_DFU_DESC                                         0x09

#define USBD_IDX_LANGID_STR                                      0x00 
#define USBD_IDX_MFC_STR                                         0x01 
#define USBD_IDX_PRODUCT_STR                                     0x02
#define USBD_IDX_SERIAL_STR                                      0x03 
#define USBD_IDX_CONFIG_STR                                      0x04 
#define USBD_IDX_INTERFACE_STR                                   0x05 
#define USBD_IDX_OS_STR                                          0xEE 

#define USB_REQ_TYPE_STANDARD                                    0x00
#define USB_REQ_TYPE_CLASS                                       0x20
#define USB_REQ_TYPE_VENDOR                                      0x40
#define USB_REQ_TYPE_MASK                                        0x60

#define USB_REQ_RECIPIENT_DEVICE                                 0x00
#define USB_REQ_RECIPIENT_INTERFACE                              0x01
#define USB_REQ_RECIPIENT_ENDPOINT                               0x02
#define USB_REQ_RECIPIENT_MASK                                   0x03

#define USB_REQ_GET_STATUS                                       0x00
#define USB_REQ_CLEAR_FEATURE                                    0x01
#define USB_REQ_SET_FEATURE                                      0x03
#define USB_REQ_SET_ADDRESS                                      0x05
#define USB_REQ_GET_DESCRIPTOR                                   0x06
#define USB_REQ_SET_DESCRIPTOR                                   0x07
#define USB_REQ_GET_CONFIGURATION                                0x08
#define USB_REQ_SET_CONFIGURATION                                0x09
#define USB_REQ_GET_INTERFACE                                    0x0A
#define USB_REQ_SET_INTERFACE                                    0x0B
#define USB_REQ_SYNCH_FRAME                                      0x0C
#define	USB_REQ_GET_OS_FEATURE_DESCRIPTOR                        0x20

#define USB_DESC_TYPE_DEVICE                                     1
#define USB_DESC_TYPE_CONFIGURATION                              2
#define USB_DESC_TYPE_STRING                                     3
#define USB_DESC_TYPE_INTERFACE                                  4
#define USB_DESC_TYPE_ENDPOINT                                   5
#define USB_DESC_TYPE_DEVICE_QUALIFIER                           6
#define USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION                  7
#define USB_DESC_TYPE_DFU                                        0x21
#define USB_DESC_TYPE_BOS                                        0x0F

#define USB_DESC_TYPE_OS_FEATURE_EXT_PROPERTIES                  4
#define USB_DESC_TYPE_OS_FEATURE_EXT_COMPAT_ID                   5

#define USB_DEVICE_CLASS_COMPOSITE                               0xEF
#define USB_DEVICE_CLASS_CDC                                     0x02

#define USB_DEVICE_SUBCLASS_COMPOSITE                            0x02
#define USB_DEVICE_SUBCLASS_CDC                                  0x00

#define USB_INTERFACE_CLASS_COMM_IFACE                           0x02
#define USB_INTERFACE_CLASS_CDC                                  0x0A
#define USB_INTERFACE_CLASS_APP_SPECIFIC                         0xFE

#define USB_CONFIG_REMOTE_WAKEUP                                 2
#define USB_CONFIG_SELF_POWERED                                  1

#define USB_FEATURE_EP_HALT                                      0
#define USB_FEATURE_REMOTE_WAKEUP                                1
#define USB_FEATURE_TEST_MODE                                    2

#define WINUSB_PROP_DATA_TYPE_REG_SZ                             1
#define WINUSB_PROP_DATA_TYPE_REG_EXPAND_SZ                      2
#define WINUSB_PROP_DATA_TYPE_REG_BINARY                         3
#define WINUSB_PROP_DATA_TYPE_REG_DWORD_LITTLE_ENDIAN            4
#define WINUSB_PROP_DATA_TYPE_REG_DWORD_BIG_ENDIAN               5
#define WINUSB_PROP_DATA_TYPE_REG_LINK                           6
#define WINUSB_PROP_DATA_TYPE_REG_REG_MULTI_SZ                   7

#define WINUSB_BCD_VERSION                                       0x0100
#define WINUSB_REQ_GET_COMPATIBLE_ID_FEATURE_DESCRIPTOR          0x04
#define WINUSB_REQ_GET_EXTENDED_PROPERTIES_OS_FEATURE_DESCRIPTOR 0x05


/**
  * @}
  */ 


/** @defgroup USBD_DEF_Exported_TypesDefinitions
  * @{
  */
/**
  * @}
  */ 



/** @defgroup USBD_DEF_Exported_Macros
  * @{
  */ 
#define  SWAPBYTE(addr)        (((uint16_t)(*((uint8_t *)(addr)))) + \
                               (((uint16_t)(*(((uint8_t *)(addr)) + 1))) << 8))

#define LOBYTE(x)  ((uint8_t)((x) & 0x00FF))
#define HIBYTE(x)  ((uint8_t)(((x) & 0xFF00) >>8))
/**
  * @}
  */ 

/** @defgroup USBD_DEF_Exported_Variables
  * @{
  */ 

/**
  * @}
  */ 

/** @defgroup USBD_DEF_Exported_FunctionsPrototype
  * @{
  */ 

/**
  * @}
  */ 

#endif /* __USBD_DEF_H */

/**
  * @}
  */ 

/**
* @}
*/ 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
