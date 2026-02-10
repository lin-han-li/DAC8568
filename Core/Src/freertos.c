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
#include "sd_waveform.h"
#include <stdio.h>
#include <string.h>

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

/* === DAC waveform partitions: 4MB reserved + 7 x 4MB waves === */
#define DAC_WAVE_PART_COUNT SD_DAC_QSPI_PARTITION_COUNT
#define DAC_FAULT_COUNT 6u
#define DAC_FAULT_CMD_NONE    0u
#define DAC_FAULT_CMD_TRIGGER 1u
#define DAC_FAULT_CMD_STOP    2u

static SD_DacWaveInfo_t s_dac_wave_info[DAC_WAVE_PART_COUNT];
static uint32_t s_dac_wave_ready_mask = 0u;    /* bit i => partition i header ok */
static uint32_t s_dac_wave_sd_sync_mask = 0u;  /* bit i => partition i synced from SD this boot */
static volatile uint8_t s_dac_wave_boot_sync_done = 0u;
static volatile uint8_t s_dac_stream_started = 0u;

/* Fault burst runtime state (read by UI via DAC_FaultBurst_GetUiState). */
static volatile uint8_t s_fault_active_id_0_5 = 0xFFu; /* 0xFF => normal */
static volatile TickType_t s_fault_end_tick = 0;
static volatile uint32_t s_fault_remaining_s = 0u;
static volatile uint8_t s_fault_cmd_pending = 0u;
static volatile uint8_t s_fault_cmd_type = DAC_FAULT_CMD_NONE;
static volatile uint8_t s_fault_cmd_id_0_5 = 0xFFu;
static volatile uint32_t s_fault_cmd_duration_s = 0u;

static const char * const s_dac_wave_sd_paths[DAC_WAVE_PART_COUNT] = {
  DAC_WAVE_SD_PATH_NORMAL,
  DAC_WAVE_SD_PATH_AC_COUPLING,
  DAC_WAVE_SD_PATH_BUS_GROUND,
  DAC_WAVE_SD_PATH_INSULATION,
  DAC_WAVE_SD_PATH_CAP_AGING,
  DAC_WAVE_SD_PATH_PWM_ABNORMAL,
  DAC_WAVE_SD_PATH_IGBT_FAULT,
};

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

static void dac_fault_burst_service(void);
static bool dac_fault_apply_trigger(uint32_t fault_id_0_5, uint32_t duration_s);
static void dac_fault_apply_stop(void);
static void dac_fault_post_command(uint8_t cmd_type, uint8_t fault_id_0_5, uint32_t duration_s);

/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 0 */
bool edgewind_ui_can_show_enter_button(void)
{
  return (s_dac_wave_boot_sync_done != 0u);
}
/* USER CODE END 0 */

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
  /* add threads, ... */
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
  uint32_t time;
  UBaseType_t min_hw = (UBaseType_t)0xFFFFFFFFu;
  TickType_t last_log = 0;
  //  // LVGL图形库初始化三件套
  lv_init();            // 初始化LVGL核心库（内存管理、内部变量等）
  lv_port_disp_init();  // 初始化显示驱动接口（配置帧缓冲区、注册刷新回调）
  lv_port_indev_init(); // 初始化输入设备接口（注册触摸屏/编码器驱动）
  
  /* 初始化 EdgeWind 自定义 UI */
  edgewind_ui_init();
  /* Infinite loop */
  for(;;)
  {

    /* === 临界区开始（保护LVGL操作）=== */
    osMutexAcquire(mutex_id, osWaitForever);

    /* EdgeWind UI 数据刷新 */
    edgewind_ui_refresh();
    
    /* LVGL 核心处理 */
    lv_task_handler();

    osMutexRelease(mutex_id);
    /* === 临界区结束 === */

    /* 低频监测 LVGL 线程栈水位（观察是否逼近溢出） */
    TickType_t now = xTaskGetTickCount();
    if ((now - last_log) > pdMS_TO_TICKS(2000)) {
      UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
      if (hw < min_hw) {
        min_hw = hw;
        printf("[LVGL] stack HW=%lu words, freeHeap=%lu\r\n",
               (unsigned long)hw,
               (unsigned long)xPortGetFreeHeapSize());
      }
      last_log = now;
    }

    //    /* 周期延时（关键性能参数）*/
    osDelay(LV_DEF_REFR_PERIOD + 1); // 保持屏幕刷新率稳定（典型值30ms≈33FPS）

    // 注意：
    // 1. 延时过短：导致刷新不全（屏幕闪烁）
    // 2. 延时过长：界面响应迟滞
    // 3. LV_DISP_DEF_REFR_PERIOD应与屏显参数匹配（在lv_conf.h中配置）

    // 调试语句（使用时需注意串口输出可能影响实时性）
    // printf("GUI task heartbeat\r\n");
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
  uint8_t stream_enabled = 0u;
  const uint8_t do_boot_sync = (DAC_WAVE_BOOT_FULL_SYNC != 0u) ? 1u : 0u;

  memset(s_dac_wave_info, 0, sizeof(s_dac_wave_info));
  s_dac_wave_ready_mask = 0u;
  s_dac_wave_sd_sync_mask = 0u;
  s_dac_wave_boot_sync_done = 0u;
  s_dac_stream_started = 0u;
  s_fault_active_id_0_5 = 0xFFu;
  s_fault_end_tick = 0;
  s_fault_remaining_s = 0u;
  s_fault_cmd_pending = 0u;
  s_fault_cmd_type = DAC_FAULT_CMD_NONE;
  s_fault_cmd_id_0_5 = 0xFFu;
  s_fault_cmd_duration_s = 0u;

  /* NOTE: FatFs SD driver (FATFS/Target/sd_diskio.c) gates SD_initialize() on
   * osKernelRunning(), so SD mount/sync must happen after scheduler start. */
  if (do_boot_sync != 0u) {
    printf("[DAC] init ok, waiting SD full sync in RTOS\r\n");
    printf("[DAC WAVE] full sync begin: partitions=%lu\r\n", (unsigned long)DAC_WAVE_PART_COUNT);
  } else {
    printf("[DAC] init ok, boot full sync disabled\r\n");
    printf("[DAC WAVE] boot load begin(from QSPI): partitions=%lu\r\n",
           (unsigned long)DAC_WAVE_PART_COUNT);
  }

  for (uint32_t i = 0u; i < DAC_WAVE_PART_COUNT; i++) {
    SD_DacWavePartition_t part = (SD_DacWavePartition_t)i;
    SD_DacWaveInfo_t info = {0};
    const char *path = s_dac_wave_sd_paths[i];
    if (do_boot_sync != 0u) {
      printf("[DAC WAVE] syncing partition %lu/%lu: %s\r\n",
             (unsigned long)(i + 1u),
             (unsigned long)DAC_WAVE_PART_COUNT,
             SD_Wave_GetPartitionName(part));

      if (SD_Wave_SyncDacToQspiPartition(path, part, &info)) {
        s_dac_wave_ready_mask |= (1u << i);
        s_dac_wave_sd_sync_mask |= (1u << i);
        s_dac_wave_info[i] = info;
        continue;
      }

      printf("[DAC WAVE] SD sync failed: part=%s path=%s\r\n",
             SD_Wave_GetPartitionName(part),
             (path != NULL) ? path : "(null)");
    } else {
      printf("[DAC WAVE] loading partition %lu/%lu from QSPI: %s\r\n",
             (unsigned long)(i + 1u),
             (unsigned long)DAC_WAVE_PART_COUNT,
             SD_Wave_GetPartitionName(part));
    }

    if (SD_Wave_LoadDacInfoFromQspiPartition(part, &info)) {
      s_dac_wave_ready_mask |= (1u << i);
      s_dac_wave_info[i] = info;
      printf("[DAC WAVE] load from QSPI ok: part=%s sps=%lu count=%lu addr=0x%08lX\r\n",
             SD_Wave_GetPartitionName(part),
             (unsigned long)info.sample_rate_hz,
             (unsigned long)info.sample_count,
             (unsigned long)info.qspi_mmap_addr);
      continue;
    }

    printf("[DAC WAVE] partition not ready: part=%s\r\n", SD_Wave_GetPartitionName(part));
  }

  s_dac_wave_boot_sync_done = 1u;
  if (do_boot_sync != 0u) {
    printf("[DAC WAVE] full sync done: ready_mask=0x%02lX sd_sync_mask=0x%02lX\r\n",
           (unsigned long)s_dac_wave_ready_mask,
           (unsigned long)s_dac_wave_sd_sync_mask);
  } else {
    printf("[DAC WAVE] boot load done: ready_mask=0x%02lX\r\n",
           (unsigned long)s_dac_wave_ready_mask);
  }

  if ((s_dac_wave_ready_mask & 0x1u) == 0u) {
    printf("[DAC WAVE] baseline not ready, no output\r\n");
  } else {
#if (DAC_WAVE_REQUIRE_SD_SYNC != 0)
    if ((do_boot_sync != 0u) && ((s_dac_wave_sd_sync_mask & 0x1u) == 0u)) {
      printf("[DAC WAVE] baseline requires SD sync, but SD sync failed; no output\r\n");
    } else
#endif
    {
      SD_DacWaveInfo_t *base = &s_dac_wave_info[0];
      if (DAC8568_DMA_UseQspiWave(base->qspi_mmap_addr, base->sample_count, base->sample_rate_hz) == 0) {
        printf("[DAC WAVE] baseline source=QSPI sps=%lu count=%lu addr=0x%08lX\r\n",
               (unsigned long)base->sample_rate_hz,
               (unsigned long)base->sample_count,
               (unsigned long)base->qspi_mmap_addr);
        stream_enabled = 1u;
      } else {
        printf("[DAC WAVE] baseline source switch failed, no output\r\n");
      }
    }
  }

  if (stream_enabled != 0u) {
    DAC8568_DMA_Start();
    s_dac_stream_started = 1u;
    printf("[DAC] start sps=%lu\r\n", (unsigned long)DAC_SAMPLE_RATE_HZ);
  } else {
    DAC8568_OutputFixedVoltage(0.0f);
    printf("[DAC] stream disabled (no waveform output)\r\n");
  }

  TickType_t last_log = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
    dac_fault_burst_service();
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

static uint32_t dac_fault_clamp_duration_s(uint32_t duration_s)
{
  if (duration_s < 1u) {
    return 1u;
  }
  if (duration_s > 3600u) {
    return 3600u;
  }
  return duration_s;
}

static bool dac_wave_partition_ready(uint8_t partition)
{
  if (partition >= DAC_WAVE_PART_COUNT) {
    return false;
  }
  if ((s_dac_wave_ready_mask & (1u << partition)) == 0u) {
    return false;
  }
  if (s_dac_wave_info[partition].qspi_mmap_addr == 0u || s_dac_wave_info[partition].sample_count == 0u) {
    return false;
  }
  return true;
}

static void dac_fault_post_command(uint8_t cmd_type, uint8_t fault_id_0_5, uint32_t duration_s)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  s_fault_cmd_type = cmd_type;
  s_fault_cmd_id_0_5 = fault_id_0_5;
  s_fault_cmd_duration_s = duration_s;
  s_fault_cmd_pending = 1u;
  if (primask == 0u) {
    __enable_irq();
  }
}

static bool dac_fault_apply_trigger(uint32_t fault_id_0_5, uint32_t duration_s)
{
  const uint8_t partition = (uint8_t)(fault_id_0_5 + 1u);
  const uint32_t dur_s = dac_fault_clamp_duration_s(duration_s);
  const TickType_t now = xTaskGetTickCount();
  const TickType_t delta = pdMS_TO_TICKS(dur_s * 1000u);

  if (fault_id_0_5 >= DAC_FAULT_COUNT) {
    return false;
  }
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return false;
  }
  if (!dac_wave_partition_ready(0u) || !dac_wave_partition_ready(partition)) {
    return false;
  }

  if (DAC8568_DMA_RequestQspiWave(partition,
                                  s_dac_wave_info[partition].qspi_mmap_addr,
                                  s_dac_wave_info[partition].sample_count,
                                  true) != 0) {
    return false;
  }

  s_fault_active_id_0_5 = (uint8_t)fault_id_0_5;
  s_fault_end_tick = now + delta;
  s_fault_remaining_s = dur_s;
  return true;
}

static void dac_fault_apply_stop(void)
{
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return;
  }
  if (!dac_wave_partition_ready(0u)) {
    return;
  }

  (void)DAC8568_DMA_RequestQspiWave(0u,
                                    s_dac_wave_info[0].qspi_mmap_addr,
                                    s_dac_wave_info[0].sample_count,
                                    false);

  s_fault_active_id_0_5 = 0xFFu;
  s_fault_end_tick = 0;
  s_fault_remaining_s = 0u;
}

bool DAC_FaultBurst_Trigger(uint32_t fault_id_0_5, uint32_t duration_s)
{
  const uint8_t partition = (uint8_t)(fault_id_0_5 + 1u);

  if (fault_id_0_5 >= DAC_FAULT_COUNT) {
    return false;
  }
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return false;
  }
  if (!dac_wave_partition_ready(0u) || !dac_wave_partition_ready(partition)) {
    return false;
  }
  dac_fault_post_command(DAC_FAULT_CMD_TRIGGER, (uint8_t)fault_id_0_5, duration_s);
  return true;
}

void DAC_FaultBurst_Stop(void)
{
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return;
  }
  if (!dac_wave_partition_ready(0u)) {
    return;
  }
  dac_fault_post_command(DAC_FAULT_CMD_STOP, 0xFFu, 0u);
}

void DAC_FaultBurst_GetUiState(uint32_t *ready_mask, uint8_t *active_fault_id_0_5, uint32_t *remaining_s)
{
  if (ready_mask != NULL) {
    *ready_mask = s_dac_wave_ready_mask;
  }
  if (active_fault_id_0_5 != NULL) {
    *active_fault_id_0_5 = s_fault_active_id_0_5;
  }
  if (remaining_s != NULL) {
    *remaining_s = s_fault_remaining_s;
  }
}

static void dac_fault_burst_service(void)
{
  if (s_fault_cmd_pending != 0u) {
    uint8_t cmd = DAC_FAULT_CMD_NONE;
    uint8_t fault_id = 0xFFu;
    uint32_t dur_s = 0u;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    if (s_fault_cmd_pending != 0u) {
      cmd = s_fault_cmd_type;
      fault_id = s_fault_cmd_id_0_5;
      dur_s = s_fault_cmd_duration_s;
      s_fault_cmd_pending = 0u;
      s_fault_cmd_type = DAC_FAULT_CMD_NONE;
    }
    if (primask == 0u) {
      __enable_irq();
    }

    if (cmd == DAC_FAULT_CMD_TRIGGER) {
      if (!dac_fault_apply_trigger((uint32_t)fault_id, dur_s)) {
        printf("[DAC BURST] trigger rejected: id=%lu dur=%lus\r\n",
               (unsigned long)fault_id,
               (unsigned long)dur_s);
      } else {
        printf("[DAC BURST] trigger ok: id=%lu dur=%lus\r\n",
               (unsigned long)fault_id,
               (unsigned long)dac_fault_clamp_duration_s(dur_s));
      }
    } else if (cmd == DAC_FAULT_CMD_STOP) {
      dac_fault_apply_stop();
      printf("[DAC BURST] stop\r\n");
    }
  }

  if (s_fault_active_id_0_5 == 0xFFu) {
    return;
  }
  TickType_t end = s_fault_end_tick;
  if (end == 0) {
    return;
  }

  TickType_t now = xTaskGetTickCount();
  /* Signed compare handles tick wrap-around. */
  if ((int32_t)(end - now) <= 0) {
    dac_fault_apply_stop();
    return;
  }

  uint32_t remaining_ms = (uint32_t)(end - now) * (uint32_t)portTICK_PERIOD_MS;
  uint32_t remaining_s = (remaining_ms + 999u) / 1000u;
  if (remaining_s != s_fault_remaining_s) {
    s_fault_remaining_s = remaining_s;
  }
}

/* USER CODE END Application */

