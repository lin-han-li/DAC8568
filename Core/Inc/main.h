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
#include <stdbool.h>

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

/* DAC fault burst control (implemented in Core/Src/freertos.c) */
bool DAC_FaultBurst_Trigger(uint32_t fault_id_0_5, uint32_t duration_s);
bool DAC_FaultBurst_Stop(void);
void DAC_FaultBurst_GetUiState(uint32_t *ready_mask, uint8_t *active_fault_id_0_5, uint32_t *remaining_s);
uint8_t DAC_FaultBurst_GetPartitionForFaultId(uint32_t fault_id_0_5);
bool DAC_Wave_IsBootReady(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
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
#define BEEF_Pin GPIO_PIN_7
#define BEEF_GPIO_Port GPIOH

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

#ifndef DAC_WAVE_SD_PATH_NORMAL
#define DAC_WAVE_SD_PATH_NORMAL "0:/wave/normal.bin"
#endif
#ifndef DAC_WAVE_SD_PATH_AC_COUPLING
#define DAC_WAVE_SD_PATH_AC_COUPLING "0:/wave/ac_coupling.bin"
#endif
#ifndef DAC_WAVE_SD_PATH_BUS_GROUND
#define DAC_WAVE_SD_PATH_BUS_GROUND "0:/wave/bus_ground.bin"
#endif
#ifndef DAC_WAVE_SD_PATH_INSULATION
#define DAC_WAVE_SD_PATH_INSULATION "0:/wave/insulation.bin"
#endif
#ifndef DAC_WAVE_SD_PATH_CAP_AGING
#define DAC_WAVE_SD_PATH_CAP_AGING "0:/wave/cap_aging.bin"
#endif
#ifndef DAC_WAVE_SD_PATH_PWM_ABNORMAL
#define DAC_WAVE_SD_PATH_PWM_ABNORMAL "0:/wave/pwm_abnormal.bin"
#endif
#ifndef DAC_WAVE_SD_PATH_IGBT_FAULT
#define DAC_WAVE_SD_PATH_IGBT_FAULT "0:/wave/igbt_fault.bin"
#endif

/* One-shot SD->W25Q256 sync trigger. Create this file on SD to request sync. */
#ifndef DAC_WAVE_SD_SYNC_FLAG_PATH
#define DAC_WAVE_SD_SYNC_FLAG_PATH "0:/wave/SYNC_NOW.TXT"
#endif

#ifndef DAC_WAVE_SYNC_FLAG_WAIT_MS
#define DAC_WAVE_SYNC_FLAG_WAIT_MS 12000u
#endif

#ifndef DAC_WAVE_SYNC_FLAG_RETRY_MS
#define DAC_WAVE_SYNC_FLAG_RETRY_MS 500u
#endif

/* Backward-compatible alias: baseline partition SD path */
#ifndef DAC_WAVE_SD_PATH
#define DAC_WAVE_SD_PATH DAC_WAVE_SD_PATH_NORMAL
#endif

/* 1: 必须 SD 同步成功才启动 DAC 波形流；0: 允许从 QSPI 直接加载已同步的波形或回退 */
#ifndef DAC_WAVE_REQUIRE_SD_SYNC
#define DAC_WAVE_REQUIRE_SD_SYNC 0
#endif

/*
 * Boot-time SD->W25Q256 sync gate:
 * 1: allow sync only when DAC_WAVE_SD_SYNC_FLAG_PATH exists on SD.
 * 0: never sync from SD on boot, only load existing wave metadata from QSPI.
 */
#ifndef DAC_WAVE_BOOT_FULL_SYNC
#define DAC_WAVE_BOOT_FULL_SYNC 1
#endif

/* Delete the sync trigger file after all 7 partitions sync successfully. */
#ifndef DAC_WAVE_CLEAR_SYNC_FLAG_AFTER_SUCCESS
#define DAC_WAVE_CLEAR_SYNC_FLAG_AFTER_SUCCESS 1
#endif

/* Emergency recovery switch: 1 forces one firmware build to sync on every boot. */
#ifndef DAC_WAVE_FORCE_SD_SYNC_ON_BOOT
#define DAC_WAVE_FORCE_SD_SYNC_ON_BOOT 0
#endif
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
