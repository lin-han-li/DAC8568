# EdgeWind 三项目协作关系说明

更新日期：2026-06-22

当前项目视角：

```text
本项目：STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0
角色：DAC8568 故障播放端 / HIL 故障注入源
```

本文档说明当前大创项目中三个独立项目之间的关系。三个项目不计划合并源码，也不互相覆盖目录。它们保持独立，通过文档、模型产物、波形产物和硬件接口协作。

## 1. 三个项目路径和本端目录登记

```text
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training
C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0
```

本文件属于播放端自己的仓库。播放端必须记录另外两端目录，后续互相看文档时按这些路径进入；本端不复制、不合并、不覆盖另外两端源码。

| 端 | 根目录 | 本端记录的关键入口 |
|---|---|---|
| 监测诊断端 | `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32` | `STM32H7+FreeRTOS+LVGL+ESP32`、`Edge_Wind_System`、`esp32_spi_coprocessor` |
| AI 训练端 | `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training` | `CURRENT_STATUS.md`、稳定线 `docs\v68_wind_sensor_public_fused_single_stm32_handoff.md`、V69 Aux4 `docs\v69_a4b_monitor_playback_handoff.md`、V69 RC `stm32_deploy_packages\dataset_v69_wind_sensor_aux4_public_fused_single_publicfix_single7_20260617_001615_rc` |
| 本端/播放端 | `C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0` | `NEXT_AI_PROJECT_HANDOFF.md`、`EDGEWIND_THREE_PROJECTS_CONTEXT.md`、`sd_card_payload\copy_to_sd`、`MDK-ARM\STM32H750XBH6.uvprojx` |

本端已记录的另外两端优先阅读文档：

```text
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\STM32H7+FreeRTOS+LVGL+ESP32\PROJECT_OVERVIEW.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\STM32H7+FreeRTOS+LVGL+ESP32\README.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\STM32H7+FreeRTOS+LVGL+ESP32\ESP8266_Logic_and_ESP32_WROOM32E_UE_Migration_Assessment.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\STM32H7+FreeRTOS+LVGL+ESP32\X-CUBE-AI\App\network_generate_report.txt
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\docs\three_side_handoff_monitor_stm32.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\esp32_spi_coprocessor\README.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32\Edge_Wind_System\README.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\CURRENT_STATUS.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\docs\v68_wind_sensor_public_fused_single_stm32_handoff.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\docs\v69_a4b_monitor_playback_handoff.md
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v69_wind_sensor_aux4_public_fused_single_publicfix_single7_20260617_001615_rc\README.md
```

职责划分：

| 项目 | 角色 | 主要责任 |
|---|---|---|
| `EdgeWind_STM32_ESP32` | 监测诊断端主项目 | STM32 采样、FFT、AI 诊断、ESP32 上传、服务器和 Web 展示 |
| `EdgeWind_AI_Training` | PC 端 AI 训练工程 | 生成数据、训练模型、评估模型、导出 TFLite、验证 X-CUBE-AI 部署 |
| `STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0` | DAC8568 故障播放端 | 输出 normal + 6 类故障模拟信号，作为监测端 ADC 的 HIL 故障注入源 |

## 2. 总体协作链路

```text
EdgeWind_AI_Training
    训练并导出 AI 模型
        |
        v
EdgeWind_STM32_ESP32
    监测端采样、FFT、AI 诊断、上传、Web 展示
        ^
        |
STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0
    DAC8568 输出四通道模拟故障波形
```

更具体的硬件和数据链路：

```text
AI 训练工程
    -> model_float32.tflite / preprocess.npz / golden vectors
    -> 监测端 STM32H750XBH6 X-CUBE-AI 推理

播放端 SD:/wave/*.bin
    -> W25Q256 QSPI
    -> DAC8568 A/B/C/D
    -> 监测端 ADC 输入
    -> 4ch x 4096 波形
    -> FFT / 特征提取 / AI 诊断
    -> ESP32 上传
    -> Flask / Web 展示
```

## 3. 本项目 `STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0`

本项目是故障播放端，也就是 HIL 故障注入源。它不负责 AI 诊断，不负责 ESP32 上传，不负责 Web 展示。

它的任务是把预生成的直流系统故障波形通过 DAC8568 输出给监测端 ADC。

播放链路：

```text
SD 卡 0:/wave/*.bin
    -> STM32H750XBH6
    -> W25Q256 QSPI Flash
    -> QSPI memory-mapped
    -> TIM12 + SPI1 DMA
    -> DAC8568 A/B/C/D
    -> 监测端 ADC
```

通道语义：

| DAC 通道 | 含义 |
|---|---|
| A | 正母线电压 |
| B | 负母线电压 |
| C | 负载/故障电流 |
| D | 泄漏电流 |

播放端量程契约：

| 项 | 约定 |
|---|---|
| DAC8568 实际输出 | `-5V ~ +5V` 低压模拟量 |
| A/B 工程满量程 | `-500V ~ +500V` |
| A/B 换算 | `1V` 模拟量 = `100V` 工程母线电压 |
| 正常 A/B | A 约 `+3.0V`、B 约 `-3.0V`，工程解释约 `+300V/-300V` |
| AI 训练输入 | 同一低压模拟量的 mV 表示，即正常约 `+3000mV/-3000mV` |

播放端不直接输出高压，也不把正常母线推到 `±5V` 满量程；高压语义由监测端和云端显示按比例解释。

波形文件：

```text
normal.bin
ac_coupling.bin
insulation.bin
cap_aging.bin
igbt_fault.bin
bus_ground.bin
pwm_abnormal.bin
```

波形参数：

```text
播放端采样率：102400 Hz
单文件大小：4 MB
通道数：4
数据类型：uint16 DAC code
单文件样本点：516088
数据区大小：4128704 bytes
分区尾部 guard：65536 bytes
```

本项目已完成的关键稳定性工作：

- SD 到 W25Q256 幂等同步。
- W25Q256 header/data checksum 校验。
- 断电半写入保护。
- QSPI memory-mapped 播放保护。
- DAC8568 DMA 连续播放。
- UI 六类故障触发。
- PWM/IGBT 等波形播放稳定性修复。
- H750XBH6 下载算法和 scatter 约束固化。

注意：当前播放端应保持独立，不要把旧 GUI/QSPI FatFs 资源同步逻辑重新加入主线。

## 4. `EdgeWind_STM32_ESP32` 是什么

路径：

```text
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32
```

这是监测诊断端主项目，也是三个项目中最接近“总项目入口”的目录。

它主要包含：

- STM32H750XBH6 监测端固件。
- AD7606/ADC 采样。
- `4ch x 4096` 波形窗口。
- `4ch x 2048` FFT 结果。
- ESP32 SPI 协处理器通信。
- HTTP 上传和 full frame 上报。
- Flask 服务端和 Web 监测页面。
- 故障记录、实时波形、FFT、报告展示等上层功能。

当前监测端关键基线：

```text
采样窗口：4ch x 4096
FFT：4ch x 2048
监测端采样率：25600 Hz
单窗口时长：0.16 s
典型 full frame：约 49348 bytes
```

本项目给监测端提供模拟故障输入。监测端负责判断这些模拟输入属于哪类故障。

2026-06-16 已读取的监测端文档状态：

- `STM32H7+FreeRTOS+LVGL+ESP32\PROJECT_OVERVIEW.md` 和 `README.md` 仍保留较多 ESP8266/AT 透传旧描述，可作为历史固件结构参考。
- `esp32_spi_coprocessor\README.md` 明确 ESP32-WROOM-32E/UE 的目标形态是原生通信协处理器，SPI 是 STM32<->ESP32 主业务链路，ESP32 负责 Wi-Fi、HTTP、重试和服务器命令解析。
- `Edge_Wind_System\README.md` 明确上位机是 Flask + Web UI，开发入口是 `python app.py`，本地访问 `http://localhost:5000`。
- `X-CUBE-AI\App\network_generate_report.txt` 已是 v68 生成报告，说明监测端正在向最新 3 输入单 7 类 AI 网络对齐。

## 5. `EdgeWind_AI_Training` 是什么

路径：

```text
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training
```

这是 PC 端 AI 训练与部署准备工程，不是正式监测端主工程，也不是 DAC 播放端。

它当前维护两条需要区分的状态线：

1. `CURRENT_STATUS.md`（2026-06-15）把 `dataset_v67_public_fused_1p4m_single` 记录为 production candidate。v67 是单 7 类融合模型，输入包含 `X_dwt[104]`、`X_feat[116]`、`X_spec[512,4]`、`X_rawlite[78]`，输出 `probabilities[7]`、`fault_code`、`confidence`。
2. `docs\v68_wind_sensor_public_fused_single_stm32_handoff.md`（2026-06-16）是最新监测端 STM32 handoff。v68 合同改为三输入单 7 类网络：`X_dwt[104]`、`X_feat[116]`、`X_spec[512,4]`，输出 `probabilities[7]`；没有 raw-lite、没有 E00 guard/router、没有第二模型、没有硬规则 masking。

三端联调当前应按 v68 文档记录“最新 STM32 交接产物”，但在正式烧录监测端前必须由 AI 端确认 v68 是否已经取代 v67 成为 production candidate。

v68 当前部署产物：

```text
model family:
dataset_v68_wind_sensor_public_fused_single_v6

training run:
models/dataset_v68_wind_sensor_public_fused_single_v6_single7_cpu_20260616_031448

tflite run:
models/dataset_v68_wind_sensor_public_fused_single_v6_single7_tflite_20260616_031448

stm32 deploy package:
stm32_deploy_packages/dataset_v68_wind_sensor_public_fused_single_v6_single7_20260616_031448
```

v68 X-CUBE-AI 合同：

```text
network name: network
input 1: serving_default_X_dwt0  f32(1x104)
input 2: serving_default_X_feat0 f32(1x116)
input 3: serving_default_X_spec0 f32(1x512x4)
output : nl_23                  f32(1x7)
TFLite size: 250,024 bytes
MACC: 6,216,960
weights: 238,972 B
activations: 295,904 B
```

v67 production candidate 指标摘要：

```text
dataset: 1,400,000 total, 200,000 per class
test acc: 0.999429
hil_holdout acc: 0.999400
TFLite size: 303,624 bytes
golden vectors: 21/21
```

v68 STM32 handoff 指标摘要：

```text
dataset: 350,000 total, 50,000 per class
val acc: 0.9978
test acc: 0.9978
hil_holdout acc: 0.9982
golden vectors: 21/21
Keras/TFLite max diff: 3.7252903e-09
```

模型主输出 7 类顺序保持不变：

```text
E00 normal
E01 ac_coupling
E02 insulation
E03 cap_aging
E04 igbt_fault
E05 bus_ground
E06 pwm_abnormal
```

历史 v5/v6.2/v66/v67 资料仍可作为追溯证据，但不应继续在三端交接中被写成当前唯一候选。

## 6. 三项目之间的接口约定

### 6.1 AI 训练工程到监测端

传递内容：

```text
model_float32.tflite
preprocess.npz
类别表
特征定义
golden vectors
X-CUBE-AI 生成代码
```

### 6.2 播放端到监测端

硬件接口：

```text
DAC8568 A -> 监测端 ADC 正母线电压输入
DAC8568 B -> 监测端 ADC 负母线电压输入
DAC8568 C -> 监测端 ADC 负载/故障电流输入
DAC8568 D -> 监测端 ADC 泄漏电流输入
GND 共地
```

联调顺序：

```text
normal
ac_coupling
insulation
cap_aging
igbt_fault
bus_ground
pwm_abnormal
```

### 6.3 监测端到服务器/Web

后续需要扩展上传字段：

```text
fault_code
confidence
probabilities[7]
diagnosis_latency_ms
```

### 6.4 2026-06-16 三端接口对齐

当前三端主分类接口仍然保持 7 类，不因 v67/v68 模型切换而改变：

```text
E00 normal
E01 ac_coupling
E02 insulation
E03 cap_aging
E04 igbt_fault
E05 bus_ground
E06 pwm_abnormal
```

播放端 `summary.json` 中的 `source_fault_subtypes` 只能作为 HIL 注入追溯字段使用；监测端模型当前只输出主类 `fault_code`、`confidence`、`probabilities[7]`。Web 若在 HIL 演示模式展示注入子类型，必须标注为“播放端注入信息”，不能伪装成模型预测的子类型。

四通道语义固定如下：

| 通道 | 语义 |
|---|---|
| A | 正母线电压 |
| B | 负母线电压 |
| C | 负载/变流器等效电流 |
| D | 泄漏/对地电流 |

播放端 SD payload 规则：

```text
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/igbt_fault.bin
0:/wave/bus_ground.bin
0:/wave/pwm_abnormal.bin
```

固件只把上述 7 个 `.bin` 作为 D8CW 波形解析。`summary.json` 是元数据，`SYNC_NOW.TXT` 是一次性 SD->W25Q256 同步标记；两者不能被监测端或 AI 端当作模型输入。

播放端 W25Q256 分区顺序：

```text
0 normal
1 ac_coupling
2 insulation
3 cap_aging
4 igbt_fault
5 bus_ground
6 pwm_abnormal
```

播放端 UI fault id 顺序：

```text
0 ac_coupling
1 insulation
2 cap_aging
3 igbt_fault
4 bus_ground
5 pwm_abnormal
```

量程和单位必须保持三端一致：

```text
analog_V       = DAC/ADC 实际低压模拟量，范围约 -5V ~ +5V
train_mV       = AI 训练和推理输入量，train_mV = analog_V * 1000
physical_unit  = 云端/Web/答辩展示工程量，A/B 母线 physical_bus_V = train_mV * 0.1 = analog_V * 100
```

示例：

```text
AD7606 采样 +3.0V
AI 输入 +3000mV
云端母线显示 +300V
```

播放端 `.bin` 和 DAC8568 仍按低压等效 `analog_V` 输出，不能把回放数据改成物理 `±500V` 或 `±500000mV`。监测端 AD7606 采样得到 `analog_V` 后，进入 AI 特征提取前必须乘 `1000` 变成 `train_mV`；云端/Web 显示 A/B 母线物理量时才按工程比例换算。

三端同步计划：

1. AI 端先确认 v67/v68 最终部署版本，并同步 `model_float32.tflite`、preprocess/参数、golden vectors 和 X-CUBE-AI 生成报告。
2. 监测端按选定模型合同接入输入张量；若采用 v68，只接 `X_dwt[104]`、`X_feat[116]`、`X_spec[512,4]`，不接 `X_rawlite`。
3. 播放端只负责稳定输出 canonical 7 类低压模拟波形，并通过 `summary.json` 保留注入追溯信息。
4. Web 端显示主类、置信度、工程量和规则解释；HIL 注入子类型只能作为回放端元数据展示。
5. 监测端每次替换模型前，必须重新执行 X-CUBE-AI Analyze/Generate、golden vector 对齐、实时窗口推理耗时测试和 HIL 混淆矩阵复验。

## 7. 不要做的事

三个项目保持独立，因此不要：

- 不要把三个项目源码硬合并成一个 Keil 工程。
- 不要让播放端代码覆盖监测端代码。
- 不要让 AI 训练数据进入嵌入式固件仓库。
- 不要把几十 GB 的 `data_v3/data_v4/data_v5_ablation` 直接提交到普通 git。
- 不要恢复播放端旧 QSPI FatFs GUI 资源同步逻辑。
- 不要把 H750XBH6 项目误改成 H743 配置。
- 不要丢失 `preprocess.npz`。
- 不要只看 AI golden selftest 就宣称端到端 HIL 已完成。

## 8. 后续推荐验证顺序

1. 当前稳定部署线按 AI 端 `CURRENT_STATUS.md` 和 v68 handoff 执行：`dataset_v68_wind_sensor_public_fused_single_v6`，三输入单 7 类模型，无 raw-lite、无 guard/router。
2. 监测端先完成 v68 HIL 基线验收：normal 连续 5 分钟、七类回放、记录 `probabilities[7]`、`fault_code`、`confidence` 和推理耗时。
3. 播放端准备 SD，必要时放置 `0:/wave/SYNC_NOW.TXT` 触发一次性同步 7 个波形到 W25Q256。
4. 播放端 DAC8568 A/B/C/D 接监测端 ADC，并确认 GND 共地和量程。
5. 依次播放 normal、ac_coupling、insulation、cap_aging、igbt_fault、bus_ground、pwm_abnormal。
6. 记录识别率、误报率、混淆矩阵、告警延迟和复位后 QSPI baseline 恢复情况。
7. v68 HIL 未验收通过前，不把 v69 smoke 模型或 aux4 固件改动替换到监测端主链路。

## 9. 新对话最短背景

```text
当前有三个独立项目，不计划合并源码，只要求彼此知道存在并按接口协作。

EdgeWind_STM32_ESP32 是监测诊断端主项目，负责 STM32H750XBH6 采样、FFT、AI诊断、ESP32上传、服务器和Web展示。

EdgeWind_AI_Training 是 PC 端 AI 训练和 STM32Cube.AI 部署准备工程。2026-06-15 的 CURRENT_STATUS.md 仍把 v67 dataset_v67_public_fused_1p4m_single 记为 production candidate，输入含 X_dwt[104]、X_feat[116]、X_spec[512,4]、X_rawlite[78]。2026-06-16 的 v68 STM32 handoff 是最新监测端交接产物，输入改为 X_dwt[104]、X_feat[116]、X_spec[512,4]，输出 probabilities[7]，无 raw-lite、无 E00 guard/router。正式烧录监测端前必须由 AI 端确认最终采用 v67 还是 v68。AI raw 输入单位是 train_mV，A/B 为 ±5000mV 低压模拟等效值，对应云端/Web 母线 ±500V 展示；AD7606 analog_V 进 AI 前乘1000。

STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0 是故障播放端，不是监测端。它从 SD:/wave/*.bin 同步 normal + 6 fault 到 W25Q256 七个 4MB 分区，通过 QSPI memory-mapped + TIM12 + SPI1 DMA 驱动 DAC8568 A/B/C/D 四通道输出，作为监测端 ADC 的 HIL 故障注入源。当前 W25Q 分区顺序是 normal/ac/insulation/cap/igbt/bus_ground/pwm，UI fault id 顺序是 ac/insulation/cap/igbt/bus_ground/pwm，默认触发时长 120s。
```

## 10. 2026-06-16 v69 aux4 追加规则

> 历史记录：本节描述的是早期 JSON sidecar/smoke 方案，当前 V69 正式实现已由第 11 节和 `V69_AUX4_PLAYBACK_HANDOFF.md` 覆盖。

AI 端新增 `dataset_v69_wind_sensor_aux4_public_fused_single` 作为下一集成线，但当前播放端仍以 v68 HIL 基线为主，不改变现有 7 个 D8CW 波形文件。

v69 对播放端的唯一新增候选输入是可选 sidecar：

```text
0:/wave/aux4_schedule.json
```

规则：

- `normal.bin` 到 `pwm_abnormal.bin` 仍严格保持 4 通道 D8CW，header 中 `channels=4` 不变。
- `aux4_schedule.json` 只用于低速 DAC8568 E/F/G/H 输出：
  - E -> `T_igbt_C`
  - F -> `T_dc_cap_C`
  - G -> `RH_cabinet_pct`
  - H -> `wind_load_pct`
- 没有 `aux4_schedule.json` 时，播放端必须仍能按 v68 方式播放 A/B/C/D。
- v69 smoke 模型不能作为播放端或监测端正式验收依据；必须等待 AI 端完成全量训练、正式 STM32 deploy package 和监测端 HIL 验证。
- 播放端不解析 TFLite、golden vectors 或 AI 自测文件；SD 卡仍只接受 `wave/*.bin`、`summary.json`、可选 `aux4_schedule.json` 和 `SYNC_NOW.TXT`。

## 11. 2026-06-17 V69 Aux4 正式覆盖说明

本节覆盖第 10 节的 `aux4_schedule.json` 早期 sidecar 方案。当前播放端仓库中的 V69 Aux4 权威交接文档为：

```text
V69_AUX4_PLAYBACK_HANDOFF.md
```

当前三端按以下合同对齐：

- 播放端稳定输入是 `0:/wave/aux4_schedule.a4b`；`aux4_schedule.json` 只作为可选人工检查镜像。
- AI 端 SD 卡包必须包含七个 D8CW 4 通道 `.bin`、`aux4_schedule.a4b`、`summary.json`、`SYNC_NOW.TXT`。
- 播放端把 `.a4b` 同步到 W25Q256 Aux4 双 slot：`0x00000000..0x0000FFFF` 和 `0x00010000..0x0001FFFF`；七个 4MB D8CW 分区仍从 `0x00400000` 开始。
- Aux4 一个 item 覆盖 `16384` 个 DAC 采样点，也覆盖监测端 `4096 @ 25.6 kHz` 的一个 160ms AI 窗口。
- 播放端输出边界：A/B/C/D 为高频 D8CW，E/F/G/H 为 `T_igbt_C`、`T_dc_cap_C`、`RH_cabinet_pct`、`wind_load_pct` 的 `0.5V..4.5V` 映射。
- 监测端采集边界：AD7606 ch0-ch3 走高频窗口，ch4-ch7 每 4096 点窗口求均值并反算 `X_aux[4]`。
- 当前 V69 已进入 publicfix RC 板端验收阶段；AI 端已交付 RC 包，监测端已烧录 RC，但 normal 5 分钟和七类 HIL 验收未完成前，不替换 v68 稳定回退线。

## 12. 2026-06-18 三端最新状态快照

本节只记录从 AI 端和监测端文档读到的最新状态；播放端只更新自己的文档和实现。

- AI 训练端当前 V69 publicfix RC 部署包：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v69_wind_sensor_aux4_public_fused_single_publicfix_single7_20260617_001615_rc`。
- AI 训练端匹配回放包：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v69_wind_sensor_aux4_public_fused_single_publicfix\playback_hil\dataset_v69_publicfix_rc_test_sd_g000000\wave`。
- AI 训练端匹配 golden vectors：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_test_vectors\dataset_v69_wind_sensor_aux4_public_fused_single_publicfix_20260617_001615_rc`。
- 监测端当前状态：V69 publicfix RC 已接入并烧录，用于板端验收；AD7606 ch4-ch7 Aux4 窗口均值、物理值反算和 `aux_valid` mask 已加入；ESP32/Web payload 暂不带 Aux4。
- 监测端实际 X-CUBE-AI 输入数组顺序：`X_aux[4]`、`X_dwt[104]`、`X_feat[116]`、`X_spec[512,4]`。概念上 V69 仍是 `DWT/FEAT/SPEC/AUX` 四类输入，但 STM32 绑定必须服从生成报告和 input name。
- 播放端当前边界：只输出 A/B/C/D 高频 D8CW 和 E/F/G/H Aux4 模拟量；`.a4b` 是稳定文件，`.json` 只做调试镜像；不接 TFLite、golden vectors 或 X-CUBE-AI 产物。
- 稳定性门槛：V69 publicfix RC 通过 `normal.bin` 5 分钟无反复 E01/E04，并完成七类 HIL 验收前，不能宣布替代 v68。

## 13. 2026-06-19 V70_r2 三端状态

本节记录播放端视角的最新三端关系，只修改播放端自己的文档。

- 稳定回退线仍是 v68；V69 publicfix RC 是上一条四输入 Aux4 验收线；V70_r2 是当前用户已启动监测端部署的新验收线。
- 当前不能写成 V70_r2 已完成部署：监测端文档和 `network_generate_report.txt` 仍显示 V69 publicfix，需等待监测端完成 V70_r2 生成文件替换、Keil 编译下载和串口 quick check。
- AI 端 V70_r2 RC deploy package：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v70_r2_wind_realfield_e01sep_single7_20260619_023056_rc`。
- AI 端 V70_r2 matching playback wave：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v70_wind_realistic_aux_public_single_r2\playback_hil\dataset_v70_r2_wind_realfield_e01sep_test_sd_g000000\wave`。
- V70_r2 仍使用单 `network`、7 类输出、四输入顺序 `X_aux[4]`、`X_dwt[104]`、`X_feat[116]`、`X_spec[512,4]`。
- 播放端接口不变：A/B/C/D 为七个 4 通道 D8CW `.bin`，E/F/G/H 为 `aux4_schedule.a4b` 低速 Aux4；不改 8 通道 D8CW，不接模型或 golden vectors。
- ESP32/Web payload 不因本次播放端文档对齐而变化；Aux4 仍先通过监测端内部和串口日志验证。
- 播放端固件已通过 Keil rebuild/flash，`J:\wave` 已验证可按当前固件同步输出；若正式验收 V70_r2，优先换成 AI V70_r2 matching wave 包并重新做 SD 包校验。
- V70_r2 仍是 release candidate。正式替代 v68 前必须通过 `normal.bin` 5 分钟无反复 E01/E04、七类 HIL top1 跟随主类、`aux4/aux_valid/ppermil[7]/feature_ms/infer_ms/total_ms` 记录完整。

## 14. 2026-06-21 三端历史状态：监测端 v72，SD v73

本节是播放端视角的最新三端关系记录，只修改播放端自己的文档。

- 稳定回退线仍是 v68；V69 publicfix 和 V70_r2 是历史 RC 线；监测端当前已部署的最新 RC 线是 V72，标签 `v72-single7-monitor-deployed-20260620-182036`。
- 监测端当前生成报告指向 `dataset_v72_wind_e00e01_separated_single7_final`，代码侧返回 V72 model version。V72 已有 Keil build/flash/串口 quick-read 证据，但仍需 normal 5 分钟和七类 HIL 后才能写成稳定替代。
- AI 训练端最新候选是 V73：`dataset_v73_ad7606_sync_leakage_public_single`；deploy package 为 `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v73_ad7606_sync_leakage_single7_final`；matching wave 为 `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v73_ad7606_sync_leakage_public_single\playback_hil\dataset_v73_ad7606sync_test_sd_g000000\wave`。
- V73 对播放端没有新的文件格式要求：仍是 D8CW+A4B，不改 8 通道 D8CW，不把模型文件放到播放端 SD。
- V73 对监测端有一个关键采样合同：AD7606 ch0..ch3 必须严格同步，不做 per-channel delay compensation、interpolation 或 time shifting；D leakage 允许低非零噪声/随机游走，但不能出现 E01 风格强同步线拾取。
- 2026-06-21 当时的 `J:\wave` 是 V73 回放包，已通过播放端格式校验：7 个 D8CW 4 通道 checksum 全部匹配，`aux4_schedule.a4b` `generation=1781955505`、`payload_fnv1a32=0xCC4AC198`、7 个 entry 绑定成功，`SYNC_NOW.TXT` 存在。
- 版本匹配边界：V72 正式 HIL 要用 V72 matching wave；该 V73 包只能用于验证播放端同步/输出格式，或在日志中明确标注为 V72 固件与 V73 回放包的兼容测试。V73 正式 HIL 必须等监测端完成 V73 部署后再跑。

## 15. 2026-06-22 当前主线：v74 conservative + R_iso

本节是播放端视角的最新三端关系记录，只修改播放端自己的文档。

- 当前正式回退基线仍是 v68；当前上板验收线为 `dataset_v74_ad7606_sync_riso_public_single` conservative release candidate。
- AI 训练端当前 deploy package：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v74_ad7606_sync_riso_single7_conservative`。
- AI 训练端当前 matching wave：`C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v74_ad7606_sync_riso_public_single\playback_hil\dataset_v74_ad7606sync_riso_demo_sd_g000000\wave`。
- 监测端当前文档记录：V74 conservative 已回退并下载到 STM32，保留 8 通道展示、R_iso 语义、频谱上传开关、故障快照延迟确认和 PH7 蜂鸣器反馈。
- 播放端文件结构不变：七个 D8CW `.bin` 仍是 4 通道 A/B/C/D；`aux4_schedule.a4b` 仍是 E/F/G/H 低速上下文的权威文件；不要改 8 通道 D8CW，不把模型文件放到 SD。
- 当前 Aux4 语义：E=`T_igbt_C`，F=`T_dc_cap_C`，G=`RH_cabinet_pct`，H=`R_iso_kohm`。`wind_load_pct` 是旧语义，当前 v74 不再使用。
- 监测端采样边界：AD7606 ch0..ch3 必须严格同步；ch4..ch7 每 4096 点窗口求均值并反算工程量；ch7 是 `R_iso_kohm`，不是原始电压、ADC code 或负载百分比。
- 板端验收仍要先通过 v74 golden vectors，然后跑 `normal.bin` 5 分钟，最后跑七类 HIL。若退化，保持 v68 回退线。

## 16. 2026-06-22 V74 R_iso playback fix evidence

Playback-side status after the 2026-06-22 board session:

- The `4000 kOhm` Web symptom was traced to playback output, not SD data, DAC8568 hardware, monitor ADC, upload, or Web rendering. The stale QSPI Aux4 slot was V73 (`generation=1781955505`, legacy H range `8..110`), so H stayed at the old default midpoint `2.5V`; the V74 monitor decoded that as about `4010 kOhm`.
- Current SD A4B is V74 R_iso: `dataset_v74_ad7606_sync_riso_public_single`, `generation=1782121193`, `payload_fnv1a32=0x97CBCE3E`, H range `20..8000 kOhm`.
- Playback firmware now applies `aux_lo/aux_hi/aux_default` from the A4B header after validation. It accepts legacy H `8..110` and V74 H `20..8000`, while keeping E/F/G ranges fixed.
- Playback boot now verifies Aux4 against the seven ready D8CW checksums. If QSPI Aux4 is stale, it transiently loads `0:/wave/aux4_schedule.a4b` from SD and verifies it.
- Keil rebuild and download passed: `0 Error(s), 0 Warning(s)` and `Erase Done / Programming Done / Verify OK / Application running ...`.
- Reset log evidence: QSPI V73 Aux4 failed `ready=7 verified=0`; SD transient V74 Aux4 then passed `ready=7 verified=7 all=1`, with `range_hi_x1000=[125000,115000,98000,8000000]`.
- Current runtime is correct with SD present (`source=2`, SD transient). W25Q Aux4 is still stale because `SYNC_NOW.TXT` was absent. For persistent no-SD boot, create `0:/wave/SYNC_NOW.TXT` and reset once so firmware rewrites the Aux4 dual slot.
