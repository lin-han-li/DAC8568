# STM32H750XBH6 DAC8568 FreeRTOS LVGL 9.4.0

这是当前活跃的工程说明。旧的 ESP8266 联网、800x480 LTDC 触摸屏、GUI-Guider 资源同步和 QSPI FatFs 方案已经不是主线。

## 当前主线

- MCU: STM32H750XBH6, Cortex-M7, SYSCLK 480 MHz
- RTOS: FreeRTOS, CMSIS-RTOS2 API
- UI: LVGL 9.4.0, EdgeWind_UI
- 显示: 320x240 SPI LCD, SPI6
- 输入: KEY1-KEY4 和 TIM8 编码器, LVGL keypad
- DAC: DAC8568, SPI1 32-bit TX DMA
- DAC 节拍: TIM12 TRGO + DMAMUX sync
- 波形存储: W25Q256 QSPI memory-mapped
- 波形导入: SD 卡 FatFs, 路径 `0:/wave/*.bin`

## 运行流程

1. `Core/Src/main.c` 初始化 MPU/cache、HAL 外设、SD、QSPI、SPI LCD 和 DAC8568。
2. `Core/Src/freertos.c` 创建 LVGL、LED 和 Main 三个活跃任务。
3. `Main_Task` 从 QSPI 波形分区读取 header, 必要时从 SD 同步波形。
4. LVGL EdgeWind_UI 提供 6 类故障选择和持续时间设置。
5. UI 调用 `DAC_FaultBurst_Trigger()` 投递故障命令。
6. Main 任务在安全上下文切换 DAC 波形源。
7. DAC DMA 在半缓冲或全缓冲边界切换到对应 QSPI 波形。

## 活跃目录

- `Core/Src/main.c`: 启动、硬件初始化、NVIC 优先级修正
- `Core/Src/freertos.c`: 任务编排、DAC 故障爆发状态机
- `MDK-ARM/HARDWORK/DAC8568`: DAC8568 DMA 波形输出
- `MDK-ARM/HARDWORK/SD_Card`: SD 到 QSPI 的 DAC 波形同步和分区管理
- `MDK-ARM/HARDWORK/W25Q256`: QSPI Flash 驱动
- `MDK-ARM/HARDWORK/EdgeWind_UI`: 当前 LVGL UI
- `lvgl-9.4.0/examples/porting`: LVGL 显示和输入 port
- `tools/gen_dac_fault_suite.py`: 生成 SD 卡波形文件

## 已从活跃路径清理的旧逻辑

- ESP8266 空任务不再创建，USART2 不再随主程序初始化。
- QSPI FatFs 不再暴露为 `1:/` 盘符，QSPI 由 DAC 波形存储独占。
- 800x480 GT911/FT5206 触摸驱动不再参与 Keil 活跃构建。
- GUI-Guider 生成目录保留为历史代码，但当前工程入口走 EdgeWind_UI。
- LTDC/RGB 屏相关代码不是当前显示链路。

## QSPI 仲裁规则

- DAC QSPI 波形播放只允许读取 W25Q256 memory-mapped 区。
- SD 到 QSPI 同步、擦除、写入、普通命令读写、退出 memory-map 必须先进入 `QSPI_W25Qxx_BeginCommandMode()`。
- DAC 切到 QSPI 波形前必须确认 `QSPI_W25Qxx_IsMemoryMapped()` 为真且 `QSPI_W25Qxx_IsCommandModeBusy()` 为假。
- `QSPI_W25Qxx_SetDacPlaybackActive()` 标记 DAC 正在读 memory-map，命令模式会据此拒绝擦写/abort，返回 `W25Qxx_ERROR_BUSY`。
- 不要恢复旧 `gui_resource_map.h` + QSPI FatFs 资源同步方案；它和当前 DAC 波形分区共用同一片 W25Q256，会破坏分区约束。

## SD 卡波形文件

默认波形目录:

```text
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/bus_ground.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/pwm_abnormal.bin
0:/wave/igbt_fault.bin
```

生成方式:

```powershell
python tools/gen_dac_fault_suite.py
```

生成结果放在 `sd_card_payload/copy_to_sd/wave`，复制到 SD 卡根目录即可。

## 构建入口

使用 Keil 打开:

```text
MDK-ARM/STM32H750XBH6.uvprojx
```

当前 scatter 文件:

```text
MDK-ARM/STM32H750XBH6_d2.sct
```

## 当前稳定性修复

1. SDRAM 初始化使用局部 `FMC_SDRAM_CommandTypeDef`，避免启动期野指针。
2. SD FatFs 非 4 字节对齐写路径等待 `WRITE_CPLT_MSG`，并在 DMA 写前清理 DCache。
3. SDMMC 初始化失败直接返回，让无卡启动和后续 fallback 有机会执行。
4. QSPI memory-mapped DAC 播放与擦写/同步流程已有命令模式仲裁。
5. FreeRTOS `configENABLE_FPU` 已按 Cortex-M7 FPU 工程开启。
