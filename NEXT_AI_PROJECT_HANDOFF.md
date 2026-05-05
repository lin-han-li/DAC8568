# NEXT AI 项目技术交接报告

更新时间：2026-05-05  
播放端项目：`C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0`  
监测诊断端项目：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32`  
大创项目文档：`C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0\大学生创新创业项目基于边缘计算的风电场直流系统故障监测.docx`

本文档面向下一个接手双项目联调的 AI 或工程人员。当前项目是风电场直流系统故障播放演示端，另一个 `EdgeWind_STM32_ESP32` 项目是监测诊断端。两者共同服务于大创项目《基于边缘计算的风电场直流系统故障监测》。

## 1. 项目定位与双项目关系

大创项目目标是构建一套面向风电场直流系统的边缘计算故障监测与诊断平台，覆盖交流窜入、直流母线接地、绝缘劣化、电容老化、PWM 控制异常、IGBT 故障等场景。

当前项目不是诊断端，而是故障播放演示端。它负责产生可重复、可切换、可被采样的四通道模拟故障信号，输出给监测诊断端做 ADC 采样、FFT、故障识别、上传和 Web 展示。

| 项目 | 路径 | 责任 |
| --- | --- | --- |
| 播放端 | `STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0` | 从 SD 导入波形到 W25Q256，通过 DAC8568 输出四通道模拟信号 |
| 监测诊断端 | `EdgeWind_STM32_ESP32` | STM32 采样/FFT/诊断，ESP32 上传，Flask 服务端和 Web 展示 |

联调时，播放端 DAC8568 的 A/B/C/D 输出应接入监测端 ADC 输入。播放端只负责“产生故障”，监测端负责“看见故障并诊断故障”。

## 2. 系统总体联调链路

```mermaid
flowchart LR
    A["tools/gen_dac_fault_suite.py<br/>生成 normal + 6 类故障波形"] --> B["SD 卡<br/>0:/wave/*.bin"]
    B --> C["播放端 STM32H750XBH6<br/>FatFs 读取 SD"]
    C --> D["W25Q256 QSPI Flash<br/>7 个 4MB 波形分区"]
    D --> E["QSPI memory-mapped<br/>运行期只读播放"]
    E --> F["DAC8568<br/>SPI1 DMA + TIM12 节拍"]
    F --> G["模拟输出 A/B/C/D"]
    G --> H["监测端 STM32 ADC"]
    H --> I["FFT + 故障诊断"]
    I --> J["ESP32 上传"]
    J --> K["Flask API<br/>/api/node/full_frame_bin"]
    K --> L["Web 监测/故障/报告"]
```

已知调试入口：

| 对象 | 当前入口 | 说明 |
| --- | --- | --- |
| 播放端串口 | `COM8 @ 921600` | DAPLink VCP，工程 `printf` 走 `USART1` |
| 播放端 Keil | `D:\Keil_v542\UV4\UV4.exe` | 工程文件 `MDK-ARM\STM32H750XBH6.uvprojx` |
| 监测端 STM32 | `COM7 @ 921600` | 来源于监测端交接文档，实际接线后仍需枚举确认 |
| 监测端 ESP32 | `COM4 @ 115200` | 来源于监测端交接文档 |
| 本地 Web | `http://localhost:5000/monitor` | 监测端 Flask 页面 |

本机可能同时连接多个 STM32。接手时不要只凭端口号判断设备，应结合 USB 描述、DAPLink UID、串口日志内容和 Keil 目标确认。

## 3. 播放端硬件与固件架构

| 模块 | 当前配置 |
| --- | --- |
| MCU | STM32H750XBH6，Cortex-M7 |
| RTOS | FreeRTOS，CMSIS-RTOS2 API |
| UI | LVGL 9.4.0，当前主线是 `EdgeWind_UI` |
| 显示 | 320 x 240 SPI LCD，SPI6 |
| 输入 | KEY1-KEY4 + TIM8 编码器，LVGL keypad |
| DAC | TI DAC8568，SPI1 32-bit TX DMA |
| DAC 节拍 | TIM12 TRGO + DMAMUX sync |
| 波形存储 | W25Q256 QSPI，memory-mapped 只读播放 |
| 波形导入 | SD 卡 FatFs，入口 `0:/wave/*.bin` |

关键代码入口：

| 类型 | 路径 |
| --- | --- |
| Keil 工程 | `MDK-ARM\STM32H750XBH6.uvprojx` |
| Scatter | `MDK-ARM\STM32H750XBH6_d2.sct` |
| DAC 播放核心 | `MDK-ARM\HARDWORK\DAC8568\dac8568_dma.c` |
| SD/QSPI 同步 | `MDK-ARM\HARDWORK\SD_Card\sd_waveform.c` |
| W25Q256 驱动 | `MDK-ARM\HARDWORK\W25Q256\qspi_w25q256.c` |
| UI 故障模型 | `MDK-ARM\HARDWORK\EdgeWind_UI\components\ew_ui_fault_model.c` |
| 波形生成脚本 | `tools\gen_dac_fault_suite.py` |

Keil 配置必须保持 H750 语义：

```text
Device: STM32H750XBHx
Define: USE_HAL_DRIVER, STM32H750xx, ARM_MATH_CM7, USE_PWR_LDO_SUPPLY, LV_CONF_INCLUDE_SIMPLE
Flash algorithm: ..\STM32H7x_2048.FLM
```

不要把项目改成 H743，不要按 H743 2MB 内部 Flash 假设重配内存或下载算法。用户已经确认 `STM32H7x_2048.FLM` 是当前可用下载算法。

非当前主线：

- ESP8266 联网和 USART2 上报不是当前播放主线。
- GUI-Guider 生成页面不是当前 UI 入口。
- LTDC/RGB 屏和触摸链路不是当前硬件入口。
- 旧 `gui_resource_map.h`、旧 QSPI FatFs `1:/` 和 GUI 资源同步方案不要恢复。
- `share_to_gemini_100` 是用户删除的旧分享目录，不要擅自恢复。

## 4. 故障波形数据体系

SD 卡目标路径固定为：

```text
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/bus_ground.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/pwm_abnormal.bin
0:/wave/igbt_fault.bin
```

波形文件格式：

| 参数 | 当前值 |
| --- | --- |
| 采样率 | `102400 Hz` |
| 单文件大小 | `4194304 bytes`，即 4MB |
| 通道数 | 4 |
| 样本类型 | 每通道 `uint16` DAC code |
| 单文件样本点 | `524280` |
| 文件结构 | `SD_DacWaveHeader_t` + A/B/C/D 交织数据 |

通道语义：

| 通道 | 语义 |
| --- | --- |
| A | 正母线电压 |
| B | 负母线电压 |
| C | 负载/故障电流 |
| D | 泄漏电流 |

故障文件说明：

| 分区 | 文件名 | UI 名称 | 工程含义 | 监测端预期观察 |
| --- | --- | --- | --- | --- |
| 0 | `normal.bin` | 正常基准 | 正常母线与负载状态 | 弱纹波，低风险或正常 |
| 1 | `ac_coupling.bin` | 交流窜入 | 交流分量耦合进直流系统 | 工频/谐波增强，泄漏轻微抬升 |
| 2 | `bus_ground.bin` | 母线接地 | 母线接地或支路接地 | 电压塌陷、电流冲击、泄漏尖峰 |
| 3 | `insulation.bin` | 绝缘劣化 | 绝缘下降、材料老化或受潮 | 泄漏趋势升高，局放型脉冲 |
| 4 | `cap_aging.bin` | 电容老化 | ESR 增大、容量下降 | DC-link ripple 增强，充放电脉冲 |
| 5 | `pwm_abnormal.bin` | PWM 异常 | 占空比抖动、丢脉冲、控制异常 | 载波及边带异常，电流调制 |
| 6 | `igbt_fault.bin` | IGBT 故障 | 短路、退饱和、保护钳位 | 瞬态尖峰、电流突增、保护恢复 |

本次验证中，7 个本地 SD 波形文件均为 4MB，采样率 `102400 Hz`，4 通道，checksum 全部通过。

## 5. W25Q256 分区与同步机制

当前分区从 W25Q256 的 `0x00400000` 开始，前 4MB 保留不用：

```text
0x00400000 - 0x007FFFFF  normal
0x00800000 - 0x00BFFFFF  ac_coupling
0x00C00000 - 0x00FFFFFF  bus_ground
0x01000000 - 0x013FFFFF  insulation
0x01400000 - 0x017FFFFF  cap_aging
0x01800000 - 0x01BFFFFF  pwm_abnormal
0x01C00000 - 0x01FFFFFF  igbt_fault
```

关键开关在 `Core\Inc\main.h`：

```c
#define DAC_WAVE_REQUIRE_SD_SYNC 0
#define DAC_WAVE_BOOT_FULL_SYNC 1
```

当前策略是“演示稳定优先”：

- `DAC_WAVE_BOOT_FULL_SYNC=1`：每次启动仍会检查 SD 上的 7 个文件，便于带新 SD 文件上电后自动更新 W25Q256。
- 同步已改为幂等同步：如果 W25Q256 header 和 SD header 一致，并且 W25Q256 数据 checksum 匹配，则打印 `sync skip/already current`，不会擦写该分区。
- 只有 W25Q256 缺失、checksum 不一致或 SD 文件更新时，才擦除和重写对应分区。
- 写入流程已改为“先写数据、校验 QSPI 数据、最后写 header”，避免中途断电后半包数据被误判为 ready。
- `SD_Wave_LoadDacInfoFromQspiPartition()` 也会校验整段 QSPI 数据，header 对但数据坏时不会置 ready。
- `DAC_WAVE_REQUIRE_SD_SYNC=0`：如果 SD 卡偶发失败，但 W25Q256 里已有完整有效数据，允许从 QSPI 直接启动 baseline，避免 UI 显示 `normal:未就绪`。

最近一次 COM8 实测结果：

```text
[WAVE] sync skip/already current: part=normal(0) checksum=0x35B7277B addr=0x90400040
[WAVE] sync skip/already current: part=ac_coupling(1) checksum=0x9C4214E8 addr=0x90800040
[WAVE] sync skip/already current: part=bus_ground(2) checksum=0x194F90CE addr=0x90C00040
[WAVE] sync skip/already current: part=insulation(3) checksum=0x88B8AE37 addr=0x91000040
[WAVE] sync skip/already current: part=cap_aging(4) checksum=0x7256CAF9 addr=0x91400040
[WAVE] sync skip/already current: part=pwm_abnormal(5) checksum=0xA3E5B3CA addr=0x91800040
[WAVE] sync skip/already current: part=igbt_fault(6) checksum=0x3336670E addr=0x91C00040
[DAC WAVE] full sync done: ready_mask=0x7F sd_sync_mask=0x7F
[DAC WAVE] baseline source=QSPI sps=102400 count=524280 addr=0x90400040
[DAC] start sps=102400
[DAC] ok=... fail=0 skip=0 rec=0 reason=0 ready=0x7F sd=0x7F boot=1 stream=1 src=0 mmap=1 busy=0
```

成功判据：

- 7 个分区均 `sync ok` 或 `sync skip/already current`。
- `ready_mask=0x7F`。
- `baseline source=QSPI`。
- 周期日志持续 `ready=0x7F stream=1 mmap=1 busy=0`。

## 6. UI 与故障触发流程

当前 UI 的 6 个故障卡片定义在 `ew_ui_fault_model.c`：

```text
0: 交流窜入
1: 母线接地
2: 绝缘劣化
3: 电容老化
4: PWM异常
5: IGBT故障
```

故障详情页提供：

- 默认故障持续时间 `10s`。
- `-` / `+` 调整持续时间。
- `触发` 调用 `DAC_FaultBurst_Trigger(fault_id_0_5, duration_s)`。
- `停止` 调用 `DAC_FaultBurst_Stop()`。

运行期触发只切换 QSPI memory-mapped 波形指针：

- 不读 SD 卡。
- 不擦写 QSPI。
- 不恢复 GUI 资源同步。
- 不改变 W25Q256 分区布局。

本次已强化触发日志。PWM 故障触发成功时应出现类似：

```text
[DAC BURST] request: id=4 part=5 addr=0x91800040 count=524280 ret=0
[DAC BURST] trigger ok: id=4 dur=10s
```

故障结束或停止后会切回 normal：

```text
[DAC BURST] baseline request: addr=0x90400040 count=524280 ret=0
```

UI 状态读取已改为临界区快照，避免 UI 任务读到不一致的 `ready_mask`、当前故障 ID 或剩余时间。

## 7. QSPI 与 DAC 播放保护

DAC 播放通过 SPI1 DMA 连续输出，波形数据来自 QSPI memory-mapped 地址。播放期间 QSPI 命令模式必须受保护。

当前保护规则：

- `QSPI_W25Qxx_SetDacPlaybackActive(1)` 表示 DAC 正在读取 QSPI memory map。
- QSPI 擦写、普通读写、退出 memory map 前必须获取 command owner。
- 如果 DAC 正在播放，命令模式会返回 busy，避免运行期擦写破坏 DAC 输出。
- `DAC8568_DMA_Service()` 发现 SPI error 或 stagnant 时，会停止 TIM/SPI、解除播放 guard、强制重建 QSPI memory-mapped，然后重启 DAC stream。

新增恢复入口：

```c
int8_t QSPI_W25Qxx_ForceMemoryMapped(void);
```

该接口用于 DAC 恢复路径，不应用来恢复旧 QSPI FatFs 文件系统方案。

## 8. 监测诊断端项目概览

监测诊断端固定路径：

```text
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32
```

优先阅读其交接文档：

```text
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\NEXT_AI_HANDOFF_TECHNICAL_STATUS_20260504.md
```

当前稳定链路：

| 子系统 | 责任 |
| --- | --- |
| STM32 监测固件 | ADC 采样、FFT、诊断算法、SPI full snapshot |
| ESP32 协处理器 | WiFi、HTTP 上传、接收 STM32 full frame、返回服务器命令 |
| Flask 服务端 | `/api/node/full_frame_bin` 解码、WebSocket/HTTP 展示、故障记录 |
| Web 前端 | 实时波形、FFT、故障、历史、报告 |

监测端当前稳定结论：

```text
4ch x 4096 waveform + 4ch x 2048 FFT
POST /api/node/full_frame_bin
Content-Type: application/octet-stream
payload length: 49348 bytes
stable full upload interval: about 2.0s/frame
```

播放端联调时，监测端应接收真实 ADC 输入，而不是只跑 `sim.py`。`sim.py` 只能验证 Web 或服务端，不代表 DAC8568 输出已经被监测端正确诊断。

## 9. 双项目联调流程

播放端：

1. 生成或确认 `sd_card_payload\copy_to_sd\wave\*.bin`。
2. 复制到 SD 卡根目录，形成 `SD:/wave/*.bin`。
3. 插回 SD 卡。
4. Keil 全量编译并下载：

```powershell
cd C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0
$proj = (Resolve-Path '.\MDK-ARM\STM32H750XBH6.uvprojx').Path
& 'D:\Keil_v542\UV4\UV4.exe' -r $proj -t 'STM32H750XBH6' -o '.\MDK-ARM\rebuild.log'
& 'D:\Keil_v542\UV4\UV4.exe' -f $proj -t 'STM32H750XBH6' -o '.\MDK-ARM\flash.log'
```

5. 打开 `COM8 @ 921600`，确认 `ready=0x7F` 和 `baseline source=QSPI`。

监测端：

1. 启动或烧录 STM32 与 ESP32。
2. 启动 Flask 服务端：

```powershell
cd C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\Edge_Wind_System
.\venv311\Scripts\activate
python app.py
```

3. 打开 `http://localhost:5000/monitor`。
4. 确认 full frame 持续刷新。

联调步骤：

1. 先保持播放端 normal 输出，确认监测端波形稳定。
2. 依次触发交流窜入、母线接地、绝缘劣化、电容老化、PWM 异常、IGBT 故障。
3. 每次记录播放端串口、监测端波形、FFT、故障识别、Web 日志和报告。
4. 每个故障结束后确认系统能回到 normal。

## 10. 常见问题与风险边界

- UI 显示 `normal:未就绪` 时，优先看 COM8 周期日志里的 `ready=0x..` 和启动日志里的 QSPI checksum。
- COM8 波特率是 `921600`，用 `115200` 会看到乱码。
- 启动慢是正常现象，但当前幂等同步会跳过已一致分区，不应每次都重擦 7 个 4MB 分区。
- SD 卡拔掉时，如果 W25Q256 里已有有效数据，baseline 仍应可从 QSPI 启动。
- 播放期间不要恢复旧 QSPI FatFs 或 GUI 资源同步。
- 播放端不承担诊断算法、报告生成或服务器上传。
- 不要把 H750XBH6 工程按 H743 内部 Flash 条件改造。
- 不要恢复用户删除的 `share_to_gemini_100`。

## 11. 给下一个 AI 的接手清单

先确认播放端：

```powershell
cd C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0
git status --short --branch
```

检查点：

- Keil 工程仍是 `MDK-ARM\STM32H750XBH6.uvprojx`。
- `Core\Inc\main.h` 中 `DAC_WAVE_REQUIRE_SD_SYNC=0`，`DAC_WAVE_BOOT_FULL_SYNC=1`。
- `COM8 @ 921600` 能看到 `ready=0x7F`。
- 7 个 W25Q256 分区同步日志为 `sync ok` 或 `sync skip/already current`。
- PWM 触发日志应有 `id=4 part=5 ret=0`。

再确认监测端：

```powershell
cd C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32
git status --short --branch
```

检查点：

- 阅读 `NEXT_AI_HANDOFF_TECHNICAL_STATUS_20260504.md`。
- 服务器、ESP32、STM32 上传链路版本匹配。
- Web 页面能看到 `4ch x 4096 waveform + 4ch x 2048 FFT`。
- `/api/node/full_frame_bin` 能持续接收真实 full frame。

调试优先级：

1. 播放端 `ready=0x7F` 和 DAC baseline 输出。
2. 模拟接线和 ADC 量程。
3. 监测端 ADC 原始波形。
4. FFT 特征。
5. 故障诊断阈值或算法。
6. ESP32 上传。
7. Web 展示和报告。

最小验收标准：

- 播放端启动后 7 分区 ready。
- UI 六类故障可触发，并能自动回 normal。
- 监测端能看到真实波形和 FFT 变化。
- 至少一类故障能进入监测端故障记录或报告。
- 复位后不会因为同步中断导致 W25Q256 数据被误判为 ready，也不会反复重擦已一致分区。

## 12. 本次交接版本验证记录

本次固件变更已完成以下验证：

```text
Keil rebuild:
"STM32H750XBH6\STM32H750XBH6.axf" - 0 Error(s), 0 Warning(s).

Keil flash:
Erase Done.Programming Done.Verify OK.Application running ...

COM8:
[WAVE] sync skip/already current: part=normal(0) checksum=0x35B7277B addr=0x90400040
[WAVE] sync skip/already current: part=pwm_abnormal(5) checksum=0xA3E5B3CA addr=0x91800040
[DAC WAVE] full sync done: ready_mask=0x7F sd_sync_mask=0x7F
[DAC WAVE] baseline source=QSPI sps=102400 count=524280 addr=0x90400040
[DAC] ... ready=0x7F sd=0x7F boot=1 stream=1 src=0 mmap=1 busy=0
```

建议把本次状态作为“幂等 SD->W25Q 同步 + DAC 播放恢复 + 双项目交接文档”的稳定标签。
