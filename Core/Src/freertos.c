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
#include "DAC8568/dac8568_aux4.h"
#include "DAC8568/dac8568_dma.h"
#include "qspi_w25q256.h"
#include "sd_waveform.h"
#include "SD.h"
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
#define DAC_FAULT_CMD_QUEUE_LEN 8u
#define DAC_WAVE_ALL_PART_MASK ((uint32_t)((1u << DAC_WAVE_PART_COUNT) - 1u))

typedef struct {
  uint8_t type;
  uint8_t fault_id_0_5;
  uint16_t reserved;
  uint32_t duration_s;
} DacFaultCommand_t;

static SD_DacWaveInfo_t s_dac_wave_info[DAC_WAVE_PART_COUNT];
static uint32_t s_dac_wave_ready_mask = 0u;    /* bit i => partition i header ok */
static uint32_t s_dac_wave_sd_sync_mask = 0u;  /* bit i => partition i synced from SD this boot */
static volatile uint8_t s_dac_wave_boot_sync_done = 0u;
static volatile uint8_t s_dac_stream_started = 0u;

/* Fault burst runtime state (read by UI via DAC_FaultBurst_GetUiState). */
static volatile uint8_t s_fault_active_id_0_5 = 0xFFu; /* 0xFF => normal */
static volatile TickType_t s_fault_end_tick = 0;
static volatile uint32_t s_fault_remaining_s = 0u;
static DacFaultCommand_t s_fault_cmd_queue[DAC_FAULT_CMD_QUEUE_LEN];
static volatile uint8_t s_fault_cmd_head = 0u;
static volatile uint8_t s_fault_cmd_tail = 0u;
static volatile uint8_t s_fault_cmd_count = 0u;

/*
 * Canonical cross-project order:
 * E01 ac_coupling, E02 insulation, E03 cap_aging,
 * E04 igbt_fault, E05 bus_ground, E06 pwm_abnormal.
 */
static const uint8_t s_dac_fault_partition_by_id[DAC_FAULT_COUNT] = {
  SD_DAC_WAVE_PART_AC_COUPLING,
  SD_DAC_WAVE_PART_INSULATION,
  SD_DAC_WAVE_PART_CAP_AGING,
  SD_DAC_WAVE_PART_IGBT_FAULT,
  SD_DAC_WAVE_PART_BUS_GROUND,
  SD_DAC_WAVE_PART_PWM_ABNORMAL,
};

static const char * const s_dac_fault_ai_code_by_id[DAC_FAULT_COUNT] = {
  "E01",
  "E02",
  "E03",
  "E04",
  "E05",
  "E06",
};

static const char * const s_dac_wave_ai_code_by_part[DAC_WAVE_PART_COUNT] = {
  "E00",
  "E01",
  "E02",
  "E03",
  "E04",
  "E05",
  "E06",
};

static const char * const s_dac_wave_sd_paths[DAC_WAVE_PART_COUNT] = {
  DAC_WAVE_SD_PATH_NORMAL,
  DAC_WAVE_SD_PATH_AC_COUPLING,
  DAC_WAVE_SD_PATH_INSULATION,
  DAC_WAVE_SD_PATH_CAP_AGING,
  DAC_WAVE_SD_PATH_IGBT_FAULT,
  DAC_WAVE_SD_PATH_BUS_GROUND,
  DAC_WAVE_SD_PATH_PWM_ABNORMAL,
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

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern const osMutexAttr_t Thread_Mutex_attr;

static void dac_fault_burst_service(void);
static bool dac_fault_apply_trigger(uint32_t fault_id_0_5, uint32_t duration_s);
static void dac_fault_apply_stop(void);
static void dac_fault_queue_reset(void);
static uint8_t dac_fault_partition_for_id(uint32_t fault_id_0_5);
static bool dac_fault_post_command(uint8_t cmd_type, uint8_t fault_id_0_5, uint32_t duration_s);
static bool dac_fault_fetch_command(DacFaultCommand_t *cmd_out);
static bool dac_wave_accept_partition(uint32_t index, SD_DacWavePartition_t part, const SD_DacWaveInfo_t *info);
static bool dac_wave_sd_sync_flag_present(void);
static bool dac_wave_wait_for_sd_sync_flag(void);
static void dac_wave_try_clear_sd_sync_flag(void);
static void dac_wave_print_contract(void);
static bool dac_aux4_verify_ready_waves(void);
static int32_t dac_aux4_scale1000(float value);

/* USER CODE END FunctionPrototypes */

void LVGL_Task(void *argument);
void LED_Task(void *argument);
void Main_Task(void *argument);

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

static void RtosFatalHold(const char *reason, const signed char *task_name)
{
  const char *name = (task_name != NULL) ? (const char *)task_name : "-";

  DAC8568_DMA_StopAndHold(0.0f);
  printf("[RTOS FAULT] %s task=%s tick=%lu\r\n",
         reason,
         name,
         (unsigned long)HAL_GetTick());

  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

static bool dac_wave_sd_sync_flag_present(void)
{
  FILINFO info;
  FRESULT res;

  res = SD_Init();
  if (res != FR_OK) {
    printf("[DAC WAVE] SD sync flag check skipped: SD init=%d path=%s\r\n",
           (int)res,
           DAC_WAVE_SD_SYNC_FLAG_PATH);
    return false;
  }

  memset(&info, 0, sizeof(info));
  res = f_stat(DAC_WAVE_SD_SYNC_FLAG_PATH, &info);
  if (res == FR_OK) {
    printf("[DAC WAVE] SD sync flag found: %s size=%lu\r\n",
           DAC_WAVE_SD_SYNC_FLAG_PATH,
           (unsigned long)info.fsize);
    return true;
  }

  printf("[DAC WAVE] SD sync flag absent: %s stat=%d\r\n",
         DAC_WAVE_SD_SYNC_FLAG_PATH,
         (int)res);
  return false;
}

static bool dac_wave_wait_for_sd_sync_flag(void)
{
  uint32_t elapsed_ms = 0u;
  uint32_t retry_ms = DAC_WAVE_SYNC_FLAG_RETRY_MS;

  if (retry_ms == 0u) {
    retry_ms = 500u;
  }

  printf("[DAC WAVE] waiting SD sync flag up to %lu ms: %s\r\n",
         (unsigned long)DAC_WAVE_SYNC_FLAG_WAIT_MS,
         DAC_WAVE_SD_SYNC_FLAG_PATH);

  for (;;) {
    if (dac_wave_sd_sync_flag_present()) {
      if (elapsed_ms != 0u) {
        printf("[DAC WAVE] SD sync flag became ready after %lu ms\r\n",
               (unsigned long)elapsed_ms);
      }
      return true;
    }

    if (elapsed_ms >= DAC_WAVE_SYNC_FLAG_WAIT_MS) {
      break;
    }

    osDelay(retry_ms);
    elapsed_ms += retry_ms;
  }

  printf("[DAC WAVE] SD sync flag wait timeout after %lu ms, fallback to QSPI\r\n",
         (unsigned long)elapsed_ms);
  return false;
}

static void dac_wave_try_clear_sd_sync_flag(void)
{
#if (DAC_WAVE_CLEAR_SYNC_FLAG_AFTER_SUCCESS != 0)
  FRESULT res = f_unlink(DAC_WAVE_SD_SYNC_FLAG_PATH);
  if ((res == FR_OK) || (res == FR_NO_FILE)) {
    printf("[DAC WAVE] SD sync flag cleared: %s\r\n", DAC_WAVE_SD_SYNC_FLAG_PATH);
  } else {
    printf("[DAC WAVE] SD sync flag clear failed: %s err=%d\r\n",
           DAC_WAVE_SD_SYNC_FLAG_PATH,
           (int)res);
  }
#else
  printf("[DAC WAVE] SD sync flag kept by config: %s\r\n", DAC_WAVE_SD_SYNC_FLAG_PATH);
#endif
}

static void dac_wave_print_contract(void)
{
  printf("[DAC WAVE] canonical order: E00 normal, E01 ac_coupling, E02 insulation, E03 cap_aging, E04 igbt_fault, E05 bus_ground, E06 pwm_abnormal\r\n");
  for (uint32_t i = 0u; i < DAC_WAVE_PART_COUNT; i++) {
    SD_DacWavePartition_t part = (SD_DacWavePartition_t)i;
    printf("[DAC WAVE] contract: part=%lu code=%s name=%s path=%s\r\n",
           (unsigned long)i,
           s_dac_wave_ai_code_by_part[i],
           SD_Wave_GetPartitionName(part),
           s_dac_wave_sd_paths[i]);
  }
}

void vApplicationMallocFailedHook(void)
{
  RtosFatalHold("malloc_failed", NULL);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
  (void)xTask;
  RtosFatalHold("stack_overflow", pcTaskName);
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
    /* LVGL diagnostics disabled (periodic stack/heap prints). */

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
  uint8_t do_boot_sync = 0u;
  uint8_t sync_mutex_locked = 0u;
  uint32_t started_sps = 0u;

  memset(s_dac_wave_info, 0, sizeof(s_dac_wave_info));
  s_dac_wave_ready_mask = 0u;
  s_dac_wave_sd_sync_mask = 0u;
  s_dac_wave_boot_sync_done = 0u;
  s_dac_stream_started = 0u;
  s_fault_active_id_0_5 = 0xFFu;
  s_fault_end_tick = 0;
  s_fault_remaining_s = 0u;
  dac_fault_queue_reset();
  dac_wave_print_contract();

  /* NOTE: FatFs SD driver (FATFS/Target/sd_diskio.c) gates SD_initialize() on
   * osKernelRunning(), so SD mount/sync must happen after scheduler start. */
  if (DAC_WAVE_FORCE_SD_SYNC_ON_BOOT != 0u) {
    do_boot_sync = 1u;
    printf("[DAC WAVE] FORCE SD sync on boot enabled by firmware build\r\n");
  } else if (DAC_WAVE_BOOT_FULL_SYNC != 0u) {
    do_boot_sync = dac_wave_wait_for_sd_sync_flag() ? 1u : 0u;
  }

  if (do_boot_sync != 0u) {
    printf("[DAC] init ok, SD sync flag requested full sync in RTOS\r\n");
    printf("[DAC WAVE] full sync begin: flag=%s partitions=%lu\r\n",
           DAC_WAVE_SD_SYNC_FLAG_PATH,
           (unsigned long)DAC_WAVE_PART_COUNT);
  } else {
    printf("[DAC] init ok, SD sync not requested\r\n");
    printf("[DAC WAVE] boot load begin(from QSPI): partitions=%lu\r\n",
           (unsigned long)DAC_WAVE_PART_COUNT);
  }

  if (do_boot_sync != 0u) {
    uint32_t attempt = 0u;

    if (mutex_id) {
      osMutexAcquire(mutex_id, osWaitForever);
      sync_mutex_locked = 1u;
      printf("[DAC WAVE] locked LVGL mutex during SD->W25Q sync\r\n");
    }

    for (;;) {
      attempt++;
      s_dac_wave_ready_mask = 0u;
      s_dac_wave_sd_sync_mask = 0u;
      memset(s_dac_wave_info, 0, sizeof(s_dac_wave_info));

      printf("[DAC WAVE] full sync attempt %lu begin\r\n", (unsigned long)attempt);

      for (uint32_t i = 0u; i < DAC_WAVE_PART_COUNT; i++) {
        SD_DacWavePartition_t part = (SD_DacWavePartition_t)i;
        SD_DacWaveInfo_t info = {0};
        const char *path = s_dac_wave_sd_paths[i];

        printf("[DAC WAVE] syncing partition %lu/%lu: code=%s part=%s path=%s\r\n",
               (unsigned long)(i + 1u),
               (unsigned long)DAC_WAVE_PART_COUNT,
               s_dac_wave_ai_code_by_part[i],
               SD_Wave_GetPartitionName(part),
               (path != NULL) ? path : "(null)");

        if (SD_Wave_SyncDacToQspiPartition(path, part, &info) &&
            dac_wave_accept_partition(i, part, &info)) {
          s_dac_wave_sd_sync_mask |= (1u << i);
          continue;
        }

        printf("[DAC WAVE] SD sync failed: part=%s path=%s\r\n",
               SD_Wave_GetPartitionName(part),
               (path != NULL) ? path : "(null)");
      }

      printf("[DAC WAVE] full sync attempt %lu done: ready_mask=0x%02lX sd_sync_mask=0x%02lX\r\n",
             (unsigned long)attempt,
             (unsigned long)s_dac_wave_ready_mask,
             (unsigned long)s_dac_wave_sd_sync_mask);

      if ((s_dac_wave_sd_sync_mask & DAC_WAVE_ALL_PART_MASK) == DAC_WAVE_ALL_PART_MASK) {
        break;
      }

      printf("[DAC WAVE] full sync incomplete, system boot is blocked until sync succeeds: synced=0x%02lX expected=0x%02lX\r\n",
             (unsigned long)s_dac_wave_sd_sync_mask,
             (unsigned long)DAC_WAVE_ALL_PART_MASK);
      osDelay(2000);
    }

  } else {
    for (uint32_t i = 0u; i < DAC_WAVE_PART_COUNT; i++) {
      SD_DacWavePartition_t part = (SD_DacWavePartition_t)i;
      SD_DacWaveInfo_t info = {0};

      printf("[DAC WAVE] loading partition %lu/%lu from QSPI: code=%s part=%s\r\n",
             (unsigned long)(i + 1u),
             (unsigned long)DAC_WAVE_PART_COUNT,
             s_dac_wave_ai_code_by_part[i],
             SD_Wave_GetPartitionName(part));

      if (SD_Wave_LoadDacInfoFromQspiPartition(part, &info) &&
          dac_wave_accept_partition(i, part, &info)) {
        printf("[DAC WAVE] load from QSPI ok: code=%s part=%s sps=%lu count=%lu checksum=0x%08lX addr=0x%08lX\r\n",
               s_dac_wave_ai_code_by_part[i],
               SD_Wave_GetPartitionName(part),
               (unsigned long)info.sample_rate_hz,
               (unsigned long)info.sample_count,
               (unsigned long)info.checksum,
               (unsigned long)info.qspi_mmap_addr);
        continue;
      }

      printf("[DAC WAVE] partition not ready: part=%s\r\n", SD_Wave_GetPartitionName(part));
    }
  }

  {
    bool aux4_loaded = false;
    bool aux4_verified = false;

    if (do_boot_sync != 0u) {
      aux4_loaded = DAC8568_Aux4_SyncFromSdToQspi(DAC_WAVE_AUX4_SCHEDULE_PATH);
    }
    if (!aux4_loaded) {
      aux4_loaded = DAC8568_Aux4_LoadFromQspi();
    }
    if (aux4_loaded) {
      aux4_verified = dac_aux4_verify_ready_waves();
    }
    if (!aux4_verified) {
      printf("[AUX4] ready-wave bind not verified, trying SD transient load: %s\r\n",
             DAC_WAVE_AUX4_SCHEDULE_PATH);
      aux4_loaded = DAC8568_Aux4_LoadFromSd(DAC_WAVE_AUX4_SCHEDULE_PATH);
      if (aux4_loaded) {
        aux4_verified = dac_aux4_verify_ready_waves();
      }
    }
    if (!aux4_verified) {
      printf("[AUX4] no verified schedule for current D8CW set, aux4_source=default\r\n");
    }
  }
  if (do_boot_sync != 0u) {
    dac_wave_try_clear_sd_sync_flag();
  }
  if (sync_mutex_locked != 0u) {
    osMutexRelease(mutex_id);
    sync_mutex_locked = 0u;
    printf("[DAC WAVE] unlocked LVGL mutex after SD->W25Q sync\r\n");
  }

  s_dac_wave_boot_sync_done = 1u;
  DAC8568_Aux4_SetActiveFile(DAC_WAVE_SD_PATH_NORMAL, true);
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
        DAC8568_Aux4_SetActiveFile(DAC_WAVE_SD_PATH_NORMAL, true);
        printf("[DAC WAVE] baseline source=QSPI sps=%lu count=%lu checksum=0x%08lX addr=0x%08lX\r\n",
               (unsigned long)base->sample_rate_hz,
               (unsigned long)base->sample_count,
               (unsigned long)base->checksum,
               (unsigned long)base->qspi_mmap_addr);
        stream_enabled = 1u;
        started_sps = base->sample_rate_hz;
      } else {
        printf("[DAC WAVE] baseline source switch failed, no output\r\n");
      }
    }
  }

  if (stream_enabled != 0u) {
    DAC8568_DMA_Start();
    s_dac_stream_started = 1u;
    printf("[DAC] start sps=%lu\r\n", (unsigned long)started_sps);
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
      uint8_t active_source = 0u;
      uint8_t mmap = 0u;
      uint8_t qspi_busy = 0u;
      DAC8568_Aux4Status_t aux4_status;

      DAC8568_DMA_GetStats(&ok, &fail, &skip);
      DAC8568_DMA_GetHealth(&recover, &reason, &ref_rearm, &ref_refresh, &stagnant);
      DAC8568_Aux4_GetStatus(&aux4_status);
      active_source = DAC8568_DMA_GetActiveQspiSource();
      mmap = QSPI_W25Qxx_IsMemoryMapped();
      qspi_busy = QSPI_W25Qxx_IsCommandModeBusy();
      printf("[DAC] ok=%lu fail=%lu skip=%lu rec=%lu reason=%lu ref=%lu refresh=%lu stagnant=%lu ready=0x%02lX sd=0x%02lX boot=%u stream=%u src=%u mmap=%u busy=%u\r\n",
             (unsigned long)ok,
             (unsigned long)fail,
             (unsigned long)skip,
             (unsigned long)recover,
             (unsigned long)reason,
             (unsigned long)ref_rearm,
             (unsigned long)ref_refresh,
             (unsigned long)stagnant,
             (unsigned long)s_dac_wave_ready_mask,
             (unsigned long)s_dac_wave_sd_sync_mask,
             (unsigned)s_dac_wave_boot_sync_done,
             (unsigned)s_dac_stream_started,
             (unsigned)active_source,
             (unsigned)mmap,
             (unsigned)qspi_busy);
      printf("[AUX4] loaded=%u default=%u source=%u gen=%lu checksum=0x%08lX file=%s items=%lu item=%lu inject=%lu parse_error=%lu values_x1000=[%ld,%ld,%ld,%ld] mv=[%ld,%ld,%ld,%ld]\r\n",
             (unsigned)aux4_status.loaded,
             (unsigned)aux4_status.using_default,
             (unsigned)aux4_status.source,
             (unsigned long)aux4_status.generation,
             (unsigned long)aux4_status.payload_checksum,
             aux4_status.active_file,
             (unsigned long)aux4_status.active_item_count,
             (unsigned long)aux4_status.last_item_index,
             (unsigned long)aux4_status.inject_count,
             (unsigned long)aux4_status.parse_error_count,
             (long)dac_aux4_scale1000(aux4_status.values[0]),
             (long)dac_aux4_scale1000(aux4_status.values[1]),
             (long)dac_aux4_scale1000(aux4_status.values[2]),
             (long)dac_aux4_scale1000(aux4_status.values[3]),
             (long)dac_aux4_scale1000(aux4_status.volts[0]),
             (long)dac_aux4_scale1000(aux4_status.volts[1]),
             (long)dac_aux4_scale1000(aux4_status.volts[2]),
             (long)dac_aux4_scale1000(aux4_status.volts[3]));
      printf("[AUX4] range_lo_x1000=[%ld,%ld,%ld,%ld] range_hi_x1000=[%ld,%ld,%ld,%ld] default_x1000=[%ld,%ld,%ld,%ld]\r\n",
             (long)dac_aux4_scale1000(aux4_status.range_lo[0]),
             (long)dac_aux4_scale1000(aux4_status.range_lo[1]),
             (long)dac_aux4_scale1000(aux4_status.range_lo[2]),
             (long)dac_aux4_scale1000(aux4_status.range_lo[3]),
             (long)dac_aux4_scale1000(aux4_status.range_hi[0]),
             (long)dac_aux4_scale1000(aux4_status.range_hi[1]),
             (long)dac_aux4_scale1000(aux4_status.range_hi[2]),
             (long)dac_aux4_scale1000(aux4_status.range_hi[3]),
             (long)dac_aux4_scale1000(aux4_status.default_values[0]),
             (long)dac_aux4_scale1000(aux4_status.default_values[1]),
             (long)dac_aux4_scale1000(aux4_status.default_values[2]),
             (long)dac_aux4_scale1000(aux4_status.default_values[3]));
      last_log = now;
    }

    osDelay(5);
  }
  /* USER CODE END Main_Task */
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

static bool dac_wave_accept_partition(uint32_t index, SD_DacWavePartition_t part, const SD_DacWaveInfo_t *info)
{
  uint32_t safe_samples = 0u;

  if ((info == NULL) || (index >= DAC_WAVE_PART_COUNT)) {
    return false;
  }

  safe_samples = DAC8568_DMA_GetQspiSafeSamples(info->qspi_mmap_addr, info->sample_count);
  printf("[DAC WAVE] partition check: code=%s part=%s(%lu) path=%s header_count=%lu safe_samples=%lu checksum=0x%08lX addr=0x%08lX\r\n",
         s_dac_wave_ai_code_by_part[index],
         SD_Wave_GetPartitionName(part),
         (unsigned long)part,
         s_dac_wave_sd_paths[index],
         (unsigned long)info->sample_count,
         (unsigned long)safe_samples,
         (unsigned long)info->checksum,
         (unsigned long)info->qspi_mmap_addr);

  if (safe_samples != info->sample_count) {
    printf("[DAC WAVE] partition rejected: part=%s header_count=%lu safe_samples=%lu\r\n",
           SD_Wave_GetPartitionName(part),
           (unsigned long)info->sample_count,
           (unsigned long)safe_samples);
    return false;
  }

  s_dac_wave_info[index] = *info;
  s_dac_wave_ready_mask |= (1u << index);
  return true;
}

static bool dac_aux4_verify_ready_waves(void)
{
  bool all_verified = true;
  uint32_t ready_count = 0u;
  uint32_t verified_count = 0u;

  for (uint32_t i = 0u; i < DAC_WAVE_PART_COUNT; i++) {
    if ((s_dac_wave_ready_mask & (1u << i)) == 0u) {
      continue;
    }
    ready_count++;
    if (DAC8568_Aux4_VerifyWave((uint8_t)i,
                                s_dac_wave_info[i].sample_rate_hz,
                                s_dac_wave_info[i].sample_count,
                                s_dac_wave_info[i].checksum)) {
      verified_count++;
    } else {
      all_verified = false;
    }
  }
  printf("[AUX4] bind summary: ready=%lu verified=%lu all=%u\r\n",
         (unsigned long)ready_count,
         (unsigned long)verified_count,
         (unsigned)all_verified);
  return (ready_count > 0u) && (verified_count == ready_count);
}

bool DAC_Wave_IsBootReady(void)
{
  return (s_dac_wave_boot_sync_done != 0u) &&
         (s_dac_stream_started != 0u) &&
         dac_wave_partition_ready(0u);
}

static int32_t dac_aux4_scale1000(float value)
{
  if (value >= 0.0f) {
    return (int32_t)(value * 1000.0f + 0.5f);
  }
  return (int32_t)(value * 1000.0f - 0.5f);
}

bool edgewind_ui_can_show_enter_button(void)
{
  return DAC_Wave_IsBootReady();
}

void edgewind_ui_on_before_enter_button(void)
{
  if (!DAC_Wave_IsBootReady()) {
    edgewind_ui_log_set("SD->W25Q waveform sync...");
  }
}

static void dac_fault_queue_reset(void)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  s_fault_cmd_head = 0u;
  s_fault_cmd_tail = 0u;
  s_fault_cmd_count = 0u;
  if (primask == 0u) {
    __enable_irq();
  }
}

static bool dac_fault_post_command(uint8_t cmd_type, uint8_t fault_id_0_5, uint32_t duration_s)
{
  uint32_t primask = 0u;
  uint8_t tail = 0u;
  bool posted = false;

  if ((cmd_type != DAC_FAULT_CMD_TRIGGER) && (cmd_type != DAC_FAULT_CMD_STOP)) {
    return false;
  }

  primask = __get_PRIMASK();
  __disable_irq();

  if (cmd_type == DAC_FAULT_CMD_STOP) {
    s_fault_cmd_head = 0u;
    s_fault_cmd_tail = 0u;
    s_fault_cmd_count = 0u;
  }

  if (s_fault_cmd_count < DAC_FAULT_CMD_QUEUE_LEN) {
    tail = s_fault_cmd_tail;
    s_fault_cmd_queue[tail].type = cmd_type;
    s_fault_cmd_queue[tail].fault_id_0_5 = fault_id_0_5;
    s_fault_cmd_queue[tail].reserved = 0u;
    s_fault_cmd_queue[tail].duration_s = duration_s;
    tail++;
    if (tail >= DAC_FAULT_CMD_QUEUE_LEN) {
      tail = 0u;
    }
    s_fault_cmd_tail = tail;
    s_fault_cmd_count++;
    posted = true;
  }

  if (primask == 0u) {
    __enable_irq();
  }

  return posted;
}

static bool dac_fault_fetch_command(DacFaultCommand_t *cmd_out)
{
  uint32_t primask = 0u;
  uint8_t head = 0u;
  bool fetched = false;

  if (cmd_out == NULL) {
    return false;
  }

  primask = __get_PRIMASK();
  __disable_irq();
  if (s_fault_cmd_count > 0u) {
    head = s_fault_cmd_head;
    *cmd_out = s_fault_cmd_queue[head];
    head++;
    if (head >= DAC_FAULT_CMD_QUEUE_LEN) {
      head = 0u;
    }
    s_fault_cmd_head = head;
    s_fault_cmd_count--;
    fetched = true;
  }
  if (primask == 0u) {
    __enable_irq();
  }

  return fetched;
}

static uint8_t dac_fault_partition_for_id(uint32_t fault_id_0_5)
{
  if (fault_id_0_5 >= DAC_FAULT_COUNT) {
    return 0xFFu;
  }
  return s_dac_fault_partition_by_id[fault_id_0_5];
}

static bool dac_fault_apply_trigger(uint32_t fault_id_0_5, uint32_t duration_s)
{
  const uint8_t partition = dac_fault_partition_for_id(fault_id_0_5);
  const uint32_t dur_s = dac_fault_clamp_duration_s(duration_s);
  const TickType_t now = xTaskGetTickCount();
  const TickType_t delta = pdMS_TO_TICKS(dur_s * 1000u);
  int32_t req_ret = 0;

  if (partition >= DAC_WAVE_PART_COUNT) {
    printf("[DAC BURST] trigger reject invalid id=%lu\r\n",
           (unsigned long)fault_id_0_5);
    return false;
  }
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    printf("[DAC BURST] trigger reject state: id=%lu boot=%u stream=%u ready=0x%02lX\r\n",
           (unsigned long)fault_id_0_5,
           (unsigned)s_dac_wave_boot_sync_done,
           (unsigned)s_dac_stream_started,
           (unsigned long)s_dac_wave_ready_mask);
    return false;
  }
  if (!dac_wave_partition_ready(0u) || !dac_wave_partition_ready(partition)) {
    printf("[DAC BURST] trigger reject wave: id=%lu part=%u ready=0x%02lX normal_addr=0x%08lX fault_addr=0x%08lX\r\n",
           (unsigned long)fault_id_0_5,
           (unsigned)partition,
           (unsigned long)s_dac_wave_ready_mask,
           (unsigned long)s_dac_wave_info[0].qspi_mmap_addr,
           (unsigned long)s_dac_wave_info[partition].qspi_mmap_addr);
    return false;
  }

  req_ret = DAC8568_DMA_RequestQspiWave(partition,
                                        s_dac_wave_info[partition].qspi_mmap_addr,
                                        s_dac_wave_info[partition].sample_count,
                                        true);
  printf("[DAC BURST] request: id=%lu code=%s part=%s(%u) addr=0x%08lX count=%lu checksum=0x%08lX ret=%ld\r\n",
         (unsigned long)fault_id_0_5,
         s_dac_fault_ai_code_by_id[fault_id_0_5],
         SD_Wave_GetPartitionName((SD_DacWavePartition_t)partition),
         (unsigned)partition,
         (unsigned long)s_dac_wave_info[partition].qspi_mmap_addr,
         (unsigned long)s_dac_wave_info[partition].sample_count,
         (unsigned long)s_dac_wave_info[partition].checksum,
         (long)req_ret);
  if (req_ret != 0) {
    return false;
  }

  DAC8568_Aux4_SetActiveFile(s_dac_wave_sd_paths[partition], true);
  s_fault_active_id_0_5 = (uint8_t)fault_id_0_5;
  s_fault_end_tick = now + delta;
  s_fault_remaining_s = dur_s;
  return true;
}

static void dac_fault_apply_stop(void)
{
  int32_t req_ret = 0;

  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return;
  }
  if (!dac_wave_partition_ready(0u)) {
    return;
  }

  req_ret = DAC8568_DMA_RequestQspiWave(0u,
                                        s_dac_wave_info[0].qspi_mmap_addr,
                                        s_dac_wave_info[0].sample_count,
                                        false);
  printf("[DAC BURST] baseline request: addr=0x%08lX count=%lu checksum=0x%08lX ret=%ld\r\n",
         (unsigned long)s_dac_wave_info[0].qspi_mmap_addr,
         (unsigned long)s_dac_wave_info[0].sample_count,
         (unsigned long)s_dac_wave_info[0].checksum,
         (long)req_ret);

  if (req_ret == 0) {
    DAC8568_Aux4_SetActiveFile(DAC_WAVE_SD_PATH_NORMAL, false);
  }
  s_fault_active_id_0_5 = 0xFFu;
  s_fault_end_tick = 0;
  s_fault_remaining_s = 0u;
}

bool DAC_FaultBurst_Trigger(uint32_t fault_id_0_5, uint32_t duration_s)
{
  const uint8_t partition = dac_fault_partition_for_id(fault_id_0_5);

  if (partition >= DAC_WAVE_PART_COUNT) {
    return false;
  }
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return false;
  }
  if (!dac_wave_partition_ready(0u) || !dac_wave_partition_ready(partition)) {
    return false;
  }
  if (!dac_fault_post_command(DAC_FAULT_CMD_TRIGGER, (uint8_t)fault_id_0_5, duration_s)) {
    printf("[DAC BURST] command queue full: trigger id=%lu dur=%lus\r\n",
           (unsigned long)fault_id_0_5,
           (unsigned long)duration_s);
    return false;
  }
  return true;
}

uint8_t DAC_FaultBurst_GetPartitionForFaultId(uint32_t fault_id_0_5)
{
  return dac_fault_partition_for_id(fault_id_0_5);
}

bool DAC_FaultBurst_Stop(void)
{
  if (s_dac_wave_boot_sync_done == 0u || s_dac_stream_started == 0u) {
    return false;
  }
  if (!dac_wave_partition_ready(0u)) {
    return false;
  }
  if (!dac_fault_post_command(DAC_FAULT_CMD_STOP, 0xFFu, 0u)) {
    printf("[DAC BURST] command queue full: stop\r\n");
    return false;
  }
  return true;
}

void DAC_FaultBurst_GetUiState(uint32_t *ready_mask, uint8_t *active_fault_id_0_5, uint32_t *remaining_s)
{
  uint32_t ready_snapshot = 0u;
  uint8_t active_snapshot = 0xFFu;
  uint32_t remaining_snapshot = 0u;
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  ready_snapshot = s_dac_wave_ready_mask;
  active_snapshot = s_fault_active_id_0_5;
  remaining_snapshot = s_fault_remaining_s;
  if (primask == 0u) {
    __enable_irq();
  }

  if (ready_mask != NULL) {
    *ready_mask = ready_snapshot;
  }
  if (active_fault_id_0_5 != NULL) {
    *active_fault_id_0_5 = active_snapshot;
  }
  if (remaining_s != NULL) {
    *remaining_s = remaining_snapshot;
  }
}

static void dac_fault_burst_service(void)
{
  DacFaultCommand_t cmd = {0};

  if (dac_fault_fetch_command(&cmd)) {
    if (cmd.type == DAC_FAULT_CMD_TRIGGER) {
      if (!dac_fault_apply_trigger((uint32_t)cmd.fault_id_0_5, cmd.duration_s)) {
        printf("[DAC BURST] trigger rejected: id=%lu dur=%lus\r\n",
               (unsigned long)cmd.fault_id_0_5,
               (unsigned long)cmd.duration_s);
      } else {
        printf("[DAC BURST] trigger ok: id=%lu dur=%lus\r\n",
               (unsigned long)cmd.fault_id_0_5,
               (unsigned long)dac_fault_clamp_duration_s(cmd.duration_s));
      }
    } else if (cmd.type == DAC_FAULT_CMD_STOP) {
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

