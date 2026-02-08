/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern osMutexId_t mutex_id;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ESP8266_RX_Pin GPIO_PIN_5
#define ESP8266_RX_GPIO_Port GPIOD
#define ESP8266_TX_Pin GPIO_PIN_6
#define ESP8266_TX_GPIO_Port GPIOD
#define DC_Pin GPIO_PIN_12
#define DC_GPIO_Port GPIOG
#define DAC8568_CLR_Pin GPIO_PIN_9
#define DAC8568_CLR_GPIO_Port GPIOB
#define DAC8568_LDAC_Pin GPIO_PIN_6
#define DAC8568_LDAC_GPIO_Port GPIOA
#define DAC8568_DIN_Pin GPIO_PIN_7
#define DAC8568_DIN_GPIO_Port GPIOA
#define DAC85_CLK_Pin GPIO_PIN_5
#define DAC85_CLK_GPIO_Port GPIOA
#define BG_Pin GPIO_PIN_6
#define BG_GPIO_Port GPIOH
#define DAC8568_SYNC_Pin GPIO_PIN_4
#define DAC8568_SYNC_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
#define KEY1_Pin GPIO_PIN_12
#define KEY1_GPIO_Port GPIOB
#define KEY2_Pin GPIO_PIN_13
#define KEY2_GPIO_Port GPIOB
#define KEY3_Pin GPIO_PIN_14
#define KEY3_GPIO_Port GPIOB
#define KEY4_Pin GPIO_PIN_15
#define KEY4_GPIO_Port GPIOB

#ifndef DAC_SAMPLE_RATE_HZ
#define DAC_SAMPLE_RATE_HZ 240000u
#endif

#ifndef DAC_WAVE_SD_PATH
#define DAC_WAVE_SD_PATH "0:/wave/dac8568_wave.bin"
#endif

/* 1: 必须 SD 同步成功才启动 DAC 波形流；0: 允许从 QSPI 直接加载已同步的波形或回退 */
#ifndef DAC_WAVE_REQUIRE_SD_SYNC
#define DAC_WAVE_REQUIRE_SD_SYNC 1
#endif
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
