/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include "stm32h7xx_hal.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
// Demo 已移除，使用自定义 EdgeWind UI
#include "EdgeWind_UI/edgewind_ui.h"
#include "DAC8568/dac8568_dma.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void HAL_Delay(uint32_t Delay)
{
  if (Delay == 0U)
  {
    return;
  }

  uint32_t tickstart = HAL_GetTick();

  if ((osKernelGetState() == osKernelRunning) && (__get_IPSR() == 0U))
  {
    while ((HAL_GetTick() - tickstart) < Delay)
    {
      osDelay(1);
    }
    return;
  }

  while ((HAL_GetTick() - tickstart) < Delay)
  {
    /* busy wait before scheduler starts or in ISR */
  }
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#ifndef W25Q256_SELFTEST_ENABLE
/* 默认关闭：避免每次上电都擦写外部 Flash。
 * 需要自检时手动改为 1，或通过编译宏覆盖。
 */
#define W25Q256_SELFTEST_ENABLE 0
#endif

#if W25Q256_SELFTEST_ENABLE

#include "qspi_w25q256.h"
#include <string.h>

/* 自检等级：
 * 1: 单扇区擦除 + 单页写入 + 读回校验（最快，推荐长期保留）
 * 2: 增加跨页/跨扇区写入读回（更贴近真实存储使用）
 * 3: 预留（可在后续加入 16MB 边界/更大范围测试）
 */
#ifndef W25Q256_SELFTEST_LEVEL
/* 默认自检等级（仅在 W25Q256_SELFTEST_ENABLE=1 时生效） */
#define W25Q256_SELFTEST_LEVEL 2
#endif

/* 内存映射读取校验（可选）：开启后会将 QSPI 切到 memory-mapped 再 memcpy 读回校验。
 * 注意：memory-mapped 与 cache 行为相关，若你担心影响系统其他读写，保持 0 即可。
 */
#ifndef W25Q256_SELFTEST_USE_MEMORY_MAPPED
#define W25Q256_SELFTEST_USE_MEMORY_MAPPED 0
#endif

#define W25Q256_SELFTEST_ADDR   (0x1FF0000u) /* 32MB Flash 末尾附近，避开常用数据区 */
#define W25Q256_SELFTEST_LEN    (256u)       /* 单页测试 */
#define W25Q256_CROSS_LEN       (512u)       /* 跨页/跨扇区测试长度 */

static int8_t W25Q256_SelfTest_Small(void)
{
  uint8_t tx[W25Q256_SELFTEST_LEN];
  uint8_t rx[W25Q256_SELFTEST_LEN];
  uint32_t id;
  int8_t ret;

  ret = QSPI_W25Qxx_Init();
  id  = QSPI_W25Qxx_ReadID();
  printf("[W25Q256] init ret=%d, JEDEC=0x%06lX\r\n", (int)ret, (unsigned long)id);
  if (ret != QSPI_W25Qxx_OK)
  {
    return ret;
  }

  ret = QSPI_W25Qxx_SectorErase(W25Q256_SELFTEST_ADDR);
  printf("[W25Q256] sector erase @0x%08lX ret=%d\r\n", (unsigned long)W25Q256_SELFTEST_ADDR, (int)ret);
  if (ret != QSPI_W25Qxx_OK)
  {
    return ret;
  }

  for (uint32_t i = 0; i < W25Q256_SELFTEST_LEN; i++)
  {
    tx[i] = (uint8_t)(i ^ 0xA5u);
    rx[i] = 0;
  }

  ret = QSPI_W25Qxx_WritePage(tx, W25Q256_SELFTEST_ADDR, (uint16_t)W25Q256_SELFTEST_LEN);
  printf("[W25Q256] write page ret=%d\r\n", (int)ret);
  if (ret != QSPI_W25Qxx_OK)
  {
    return ret;
  }

  ret = QSPI_W25Qxx_ReadBuffer(rx, W25Q256_SELFTEST_ADDR, W25Q256_SELFTEST_LEN);
  printf("[W25Q256] read back ret=%d\r\n", (int)ret);
  if (ret != QSPI_W25Qxx_OK)
  {
    return ret;
  }

  if (memcmp(tx, rx, W25Q256_SELFTEST_LEN) != 0)
  {
    for (uint32_t i = 0; i < W25Q256_SELFTEST_LEN; i++)
    {
      if (tx[i] != rx[i])
      {
        printf("[W25Q256] verify FAIL @+%lu tx=0x%02X rx=0x%02X\r\n",
               (unsigned long)i, (unsigned int)tx[i], (unsigned int)rx[i]);
        break;
      }
    }
    return (int8_t)W25Qxx_ERROR_TRANSMIT;
  }

  printf("[W25Q256] selftest PASS (erase+program+readback)\r\n");
  return QSPI_W25Qxx_OK;
}

static int8_t W25Q256_SelfTest_CrossPageAndSector(void)
{
  const uint32_t base0 = W25Q256_SELFTEST_ADDR;          /* 4KB 扇区对齐 */
  const uint32_t base1 = W25Q256_SELFTEST_ADDR + 0x1000; /* 下一个 4KB 扇区 */
  const uint32_t start = base0 + 0x0F80u;                /* 故意靠近扇区末尾，触发跨扇区 */
  const uint32_t len   = W25Q256_CROSS_LEN;              /* 512B：跨页且跨扇区 */

  uint8_t tx[W25Q256_CROSS_LEN];
  uint8_t rx[W25Q256_CROSS_LEN];

  printf("[W25Q256] cross-sector test: erase 2 sectors, start=0x%08lX len=%lu\r\n",
         (unsigned long)start, (unsigned long)len);

  int8_t ret = QSPI_W25Qxx_SectorErase(base0);
  printf("[W25Q256] erase sector0 @0x%08lX ret=%d\r\n", (unsigned long)base0, (int)ret);
  if (ret != QSPI_W25Qxx_OK) return ret;

  ret = QSPI_W25Qxx_SectorErase(base1);
  printf("[W25Q256] erase sector1 @0x%08lX ret=%d\r\n", (unsigned long)base1, (int)ret);
  if (ret != QSPI_W25Qxx_OK) return ret;

  /* 擦除校验：读少量数据确认为 0xFF */
  {
    uint8_t blank[64];
    ret = QSPI_W25Qxx_ReadBuffer(blank, base0, sizeof(blank));
    if (ret != QSPI_W25Qxx_OK) return ret;
    for (uint32_t i = 0; i < sizeof(blank); i++)
    {
      if (blank[i] != 0xFFu)
      {
        printf("[W25Q256] erase verify FAIL sector0 @+%lu =0x%02X\r\n",
               (unsigned long)i, (unsigned int)blank[i]);
        return (int8_t)W25Qxx_ERROR_Erase;
      }
    }

    ret = QSPI_W25Qxx_ReadBuffer(blank, base1, sizeof(blank));
    if (ret != QSPI_W25Qxx_OK) return ret;
    for (uint32_t i = 0; i < sizeof(blank); i++)
    {
      if (blank[i] != 0xFFu)
      {
        printf("[W25Q256] erase verify FAIL sector1 @+%lu =0x%02X\r\n",
               (unsigned long)i, (unsigned int)blank[i]);
        return (int8_t)W25Qxx_ERROR_Erase;
      }
    }
    printf("[W25Q256] erase verify PASS (both sectors blank)\r\n");
  }

  for (uint32_t i = 0; i < len; i++)
  {
    tx[i] = (uint8_t)((i * 37u) ^ 0x5Au);
    rx[i] = 0;
  }

  ret = QSPI_W25Qxx_WriteBuffer(tx, start, len);
  printf("[W25Q256] write buffer (cross) ret=%d\r\n", (int)ret);
  if (ret != QSPI_W25Qxx_OK) return ret;

  ret = QSPI_W25Qxx_ReadBuffer(rx, start, len);
  printf("[W25Q256] read buffer (cross) ret=%d\r\n", (int)ret);
  if (ret != QSPI_W25Qxx_OK) return ret;

  if (memcmp(tx, rx, len) != 0)
  {
    for (uint32_t i = 0; i < len; i++)
    {
      if (tx[i] != rx[i])
      {
        printf("[W25Q256] cross verify FAIL @+%lu tx=0x%02X rx=0x%02X\r\n",
               (unsigned long)i, (unsigned int)tx[i], (unsigned int)rx[i]);
        break;
      }
    }
    return (int8_t)W25Qxx_ERROR_TRANSMIT;
  }

#if W25Q256_SELFTEST_USE_MEMORY_MAPPED
  ret = QSPI_W25Qxx_MemoryMappedMode();
  printf("[W25Q256] enter memory-mapped ret=%d\r\n", (int)ret);
  if (ret == QSPI_W25Qxx_OK)
  {
    uint8_t mm[W25Q256_CROSS_LEN];
    memcpy(mm, (uint8_t *)0x90000000u + start, len);
    if (memcmp(tx, mm, len) != 0)
    {
      printf("[W25Q256] memory-mapped verify FAIL\r\n");
      return (int8_t)W25Qxx_ERROR_TRANSMIT;
    }
    printf("[W25Q256] memory-mapped verify PASS\r\n");
  }
#endif

  printf("[W25Q256] cross-sector test PASS\r\n");
  return QSPI_W25Qxx_OK;
}

#endif /* W25Q256_SELFTEST_ENABLE */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for LVGL940 */
osThreadId_t LVGL940Handle;
const osThreadAttr_t LVGL940_attributes = {
  .name = "LVGL940",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LED */
osThreadId_t LEDHandle;
const osThreadAttr_t LED_attributes = {
  .name = "LED",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Main */
osThreadId_t MainHandle;
const osThreadAttr_t Main_attributes = {
  .name = "Main",
  .stack_size = 8192 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ESP8266 */
osThreadId_t ESP8266Handle;
const osThreadAttr_t ESP8266_attributes = {
  .name = "ESP8266",
  .stack_size = 5128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern const osMutexAttr_t Thread_Mutex_attr;

/* USER CODE END FunctionPrototypes */

void LVGL_Task(void *argument);
void LED_Task(void *argument);
void Main_Task(void *argument);
void ESP8266_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationTickHook(void);

/* USER CODE BEGIN 3 */
void vApplicationTickHook(void)
{
  /* This function will be called by each tick interrupt if
  configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
  added here, but the tick hook is called from an interrupt context, so
  code must not attempt to block, and only the interrupt safe FreeRTOS API
  functions can be used (those that end in FromISR()). */

  lv_tick_inc(1);
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  mutex_id = osMutexNew(&Thread_Mutex_attr);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of LVGL940 */
  LVGL940Handle = osThreadNew(LVGL_Task, NULL, &LVGL940_attributes);

  /* creation of LED */
  LEDHandle = osThreadNew(LED_Task, NULL, &LED_attributes);

  /* creation of Main */
  MainHandle = osThreadNew(Main_Task, NULL, &Main_attributes);

  /* creation of ESP8266 */
  ESP8266Handle = osThreadNew(ESP8266_Task, NULL, &ESP8266_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* Keep LED heartbeat alive even if LVGL display init stalls. */
  (void)osThreadSetPriority(LEDHandle, osPriorityAboveNormal);
  (void)osThreadSetPriority(LVGL940Handle, osPriorityNormal);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_LVGL_Task */
/**
  * @brief  Function implementing the LVGL940 thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_LVGL_Task */
void LVGL_Task(void *argument)
{
  /* USER CODE BEGIN LVGL_Task */
  UBaseType_t min_hw = (UBaseType_t)0xFFFFFFFFu;
  TickType_t last_log = 0;
  uint32_t last_flush = 0;
  lv_obj_t *safe_scr = NULL;
  lv_obj_t *safe_lbl = NULL;

  printf("[LVGL] task start\r\n");

  lv_init();
  printf("[LVGL] after lv_init, tick=%lu\r\n", (unsigned long)lv_tick_get());

  lv_port_disp_init();
  printf("[LVGL] after disp init\r\n");

  lv_port_indev_init();
  printf("[LVGL] after indev init\r\n");

  /* Safe fallback screen: verify LVGL flush path first. */
  safe_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(safe_scr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(safe_scr, LV_OPA_COVER, 0);
  safe_lbl = lv_label_create(safe_scr);
  lv_obj_set_style_text_color(safe_lbl, lv_color_hex(0x00FF66), 0);
  lv_label_set_text(safe_lbl, "LVGL START");
  lv_obj_align(safe_lbl, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_screen_load(safe_scr);
  lv_refr_now(NULL);
  printf("[LVGL] safe screen loaded\r\n");

  edgewind_ui_init();
  printf("[LVGL] edgewind init done\r\n");

  for(;;)
  {
    osMutexAcquire(mutex_id, osWaitForever);

    edgewind_ui_refresh();
    lv_task_handler();

    osMutexRelease(mutex_id);

    TickType_t now = xTaskGetTickCount();
    if ((now - last_log) > pdMS_TO_TICKS(2000)) {
      UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
      if (hw < min_hw) {
        min_hw = hw;
      }

      {
        uint32_t flush_now = g_lvgl_disp_flush_count;
        printf("[LVGL] hb tick=%lu flush=%lu(+%lu) stackHW=%lu freeHeap=%lu\r\n",
               (unsigned long)lv_tick_get(),
               (unsigned long)flush_now,
               (unsigned long)(flush_now - last_flush),
               (unsigned long)hw,
               (unsigned long)xPortGetFreeHeapSize());
        last_flush = flush_now;
      }

      last_log = now;
    }

    osDelay(LV_DEF_REFR_PERIOD + 1);
  }
  /* USER CODE END LVGL_Task */
}

/* USER CODE BEGIN Header_LED_Task */
/**
* @brief Function implementing the LED thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED_Task */
void LED_Task(void *argument)
{
  /* USER CODE BEGIN LED_Task */
  /* Infinite loop */
  for(;;)
  {
    LED1_Toggle;
    osDelay(500);
  }
  /* USER CODE END LED_Task */
}

/* USER CODE BEGIN Header_Main_Task */
/**
* @brief Function implementing the Main thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Main_Task */
void Main_Task(void *argument)
{
  /* USER CODE BEGIN Main_Task */
  TickType_t last_log = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
    DAC8568_DMA_Service();

    TickType_t now = xTaskGetTickCount();
    if ((now - last_log) >= pdMS_TO_TICKS(1000)) {
      uint32_t ok = 0u;
      uint32_t fail = 0u;
      uint32_t skip = 0u;
      uint32_t recover = 0u;
      uint32_t reason = 0u;
      uint32_t ref_rearm = 0u;
      uint32_t ref_refresh = 0u;
      uint32_t stagnant = 0u;

      DAC8568_DMA_GetStats(&ok, &fail, &skip);
      DAC8568_DMA_GetHealth(&recover, &reason, &ref_rearm, &ref_refresh, &stagnant);
      printf("[DAC] ok=%lu fail=%lu skip=%lu rec=%lu reason=%lu ref=%lu refresh=%lu stagnant=%lu\r\n",
             (unsigned long)ok,
             (unsigned long)fail,
             (unsigned long)skip,
             (unsigned long)recover,
             (unsigned long)reason,
             (unsigned long)ref_rearm,
             (unsigned long)ref_refresh,
             (unsigned long)stagnant);
      last_log = now;
    }

    osDelay(5);
  }
  /* USER CODE END Main_Task */
}

/* USER CODE BEGIN Header_ESP8266_Task */
/**
* @brief Function implementing the ESP8266 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ESP8266_Task */
void ESP8266_Task(void *argument)
{
  /* USER CODE BEGIN ESP8266_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ESP8266_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

