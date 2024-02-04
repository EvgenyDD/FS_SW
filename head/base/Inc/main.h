/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOC
#define SAIN0_Pin GPIO_PIN_3
#define SAIN0_GPIO_Port GPIOC
#define SAIN1_Pin GPIO_PIN_0
#define SAIN1_GPIO_Port GPIOA
#define SAIN2_Pin GPIO_PIN_1
#define SAIN2_GPIO_Port GPIOA
#define SAIN3_Pin GPIO_PIN_2
#define SAIN3_GPIO_Port GPIOA
#define SAIN4_Pin GPIO_PIN_3
#define SAIN4_GPIO_Port GPIOA
#define SAIN5_Pin GPIO_PIN_4
#define SAIN5_GPIO_Port GPIOA
#define SAIN6_Pin GPIO_PIN_5
#define SAIN6_GPIO_Port GPIOA
#define VBAT_Pin GPIO_PIN_6
#define VBAT_GPIO_Port GPIOA
#define NTC0_Pin GPIO_PIN_7
#define NTC0_GPIO_Port GPIOA
#define NTC1_Pin GPIO_PIN_4
#define NTC1_GPIO_Port GPIOC
#define RSSI_Pin GPIO_PIN_5
#define RSSI_GPIO_Port GPIOC
#define RGB0_Pin GPIO_PIN_10
#define RGB0_GPIO_Port GPIOB
#define RGB1_Pin GPIO_PIN_11
#define RGB1_GPIO_Port GPIOB
#define PWR_EN_Pin GPIO_PIN_12
#define PWR_EN_GPIO_Port GPIOB
#define FAN0_Pin GPIO_PIN_14
#define FAN0_GPIO_Port GPIOB
#define FAN1_Pin GPIO_PIN_15
#define FAN1_GPIO_Port GPIOB
#define SIN0_Pin GPIO_PIN_6
#define SIN0_GPIO_Port GPIOC
#define SIN1_Pin GPIO_PIN_7
#define SIN1_GPIO_Port GPIOC
#define SIN2_Pin GPIO_PIN_8
#define SIN2_GPIO_Port GPIOC
#define SIN3_Pin GPIO_PIN_9
#define SIN3_GPIO_Port GPIOC
#define SRV0_Pin GPIO_PIN_8
#define SRV0_GPIO_Port GPIOA
#define SRV1_Pin GPIO_PIN_9
#define SRV1_GPIO_Port GPIOA
#define SRV2_Pin GPIO_PIN_10
#define SRV2_GPIO_Port GPIOA
#define SRV3_Pin GPIO_PIN_11
#define SRV3_GPIO_Port GPIOA
#define BTN_SNS_Pin GPIO_PIN_12
#define BTN_SNS_GPIO_Port GPIOA
#define RGB2_Pin GPIO_PIN_15
#define RGB2_GPIO_Port GPIOA
#define RFM_IRQ_Pin GPIO_PIN_12
#define RFM_IRQ_GPIO_Port GPIOC
#define RFM_IRQ_EXTI_IRQn EXTI15_10_IRQn
#define RFM_CS_Pin GPIO_PIN_2
#define RFM_CS_GPIO_Port GPIOD
#define SRV4_Pin GPIO_PIN_6
#define SRV4_GPIO_Port GPIOB
#define SRV5_Pin GPIO_PIN_7
#define SRV5_GPIO_Port GPIOB
#define SRV6_Pin GPIO_PIN_8
#define SRV6_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
