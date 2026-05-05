# Project Overview

当前项目是 STM32H750XBH6 + FreeRTOS + LVGL + DAC8568 的故障波形输出固件。

## 核心目标

设备通过本地 LVGL UI 选择故障类型和持续时间，从 QSPI memory-mapped 区读取预生成的 4 通道 DAC 波形，并通过 TIM12 节拍驱动 SPI1 DMA 连续输出到 DAC8568。

## 当前架构

- `main.c`: 初始化硬件、LCD、SD、QSPI、DAC、FreeRTOS。
- `freertos.c`: 管理 LVGL 任务、Main 任务和 DAC 故障状态机。
- `EdgeWind_UI`: 当前唯一活跃 UI。
- `dac8568_dma.c`: DAC8568 SPI DMA 输出核心。
- `sd_waveform.c`: SD 波形文件同步到 W25Q256 QSPI 分区。
- `lv_port_disp.c`: 320x240 SPI LCD 显示 port。
- `lv_port_indev.c`: 按键和编码器输入 port。

## 非当前主线

以下逻辑保留在源码树中作为历史参考，但不应作为新功能入口继续扩展:

- ESP8266 联网和 USART2 上报链路
- GUI-Guider 生成页面和 GUI 资源同步
- 800x480 LTDC/RGB LCD 显示链路
- GT911/FT5206 触摸链路
- QSPI FatFs `1:/` 盘符

## 维护原则

后续优化优先围绕当前 DAC 波形主线做稳定性和实时性改进。旧 GUI、触摸、ESP 和 QSPI FatFs 方案如需恢复，应先重新设计与当前 QSPI 波形分区和 DAC DMA 读流的仲裁关系。

当前 QSPI 规则是：W25Q256 由 DAC 波形分区独占；DAC 播放期间只读 memory-mapped 区；任何擦除、写入、普通命令读写或退出 memory-map 都必须通过 `QSPI_W25Qxx_BeginCommandMode()` / `QSPI_W25Qxx_EndCommandMode()` 仲裁。
