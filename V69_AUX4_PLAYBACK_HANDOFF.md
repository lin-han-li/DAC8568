# V69 Aux4 Playback Handoff

Updated: 2026-07-01

This is the playback-side handoff document for V69 Aux4 integration. It records the contract that the AI training project and the monitor project must follow, but it does not modify either project.

Related project roots:

```text
Playback project:
C:\Users\pengjianzhong\Desktop\MY_Project\STM32H750XBH6_DAC8568_FreeRTOS_LVGL9.4.0

AI training project:
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training

Monitor project:
C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_STM32_ESP32
```

## 0. 2026-06-18 Three-Side Snapshot

Current state after re-reading the three project handoff documents:

- Playback side: this repository owns only the DAC8568 HIL playback path. Current V69 work is `.a4b` parsing, W25Q256 Aux4 dual-slot sync, and E/F/G/H low-speed DAC injection. A/B/C/D D8CW playback remains four-channel and unchanged.
- AI training side: V69 publicfix RC artifacts now exist. The matching deploy package is `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v69_wind_sensor_aux4_public_fused_single_publicfix_single7_20260617_001615_rc`; the matching playback package is `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v69_wind_sensor_aux4_public_fused_single_publicfix\playback_hil\dataset_v69_publicfix_rc_test_sd_g000000\wave`; the matching golden vectors are `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_test_vectors\dataset_v69_wind_sensor_aux4_public_fused_single_publicfix_20260617_001615_rc`.
- Monitor side: V69 publicfix RC has been integrated and flashed for board acceptance. AD7606 ch4-ch7 Aux4 window mean, physical decode, and valid mask are present. ESP32/Web payload is not extended with Aux4 yet.
- Gate: V69 publicfix RC is still an acceptance line, not a stable replacement for v68. Keep the v68 rollback line until `normal.bin` runs for 5 minutes without repeated E01/E04 and all seven HIL classes pass the agreed board criteria.

## 0.1 2026-06-19 V70_r2 Deployment-In-Progress Note

Historical cross-project status as of 2026-06-19:

- AI training side has prepared V70_r2 RC deploy package `dataset_v70_r2_wind_realfield_e01sep_single7_20260619_023056_rc`.
- The user has started monitor-side V70_r2 deployment, but playback-side documents must record it as deployment in progress, not completed. The latest monitor handoff and `network_generate_report.txt` still show V69 publicfix until the monitor side updates its generated files, Keil build/download result, and quick-check logs.
- V70_r2 keeps the same playback contract: seven high-speed D8CW `.bin` files remain strict `channels=4`, and Aux4 still comes from `aux4_schedule.a4b`. Do not convert D8CW to eight channels.
- Playback SD card still must not contain TFLite, X-CUBE-AI files, golden vectors, or AI self-test payloads.

Playback-side board status already achieved:

- Keil target `STM32H750XBH6` rebuild passed with `0 Error(s), 0 Warning(s)`.
- Keil flash log showed `Erase Done`, `Programming Done`, `Verify OK`, and `Application running ...`.
- Audit after build reported `RW_D2SRAM free=0x20`; do not add more D2 SRAM DMA buffers or large D2 arrays.

2026-06-19 verified SD package status:

- `J:\wave` has been checked as a valid D8CW+A4B package for the current playback firmware.
- All seven D8CW files are `4194304` bytes, `sample_rate_hz=102400`, `channels=4`, `sample_count=516088`, `data_offset=64`, and their payload checksums match the playback firmware checksum algorithm.
- `aux4_schedule.a4b` is `4512` bytes, `generation=1781631114`, `payload_fnv1a32=0xEA8E68F7`, and all seven entries bind to the matching D8CW checksums.
- `SYNC_NOW.TXT` exists in `J:\wave`, so the next board boot with that SD card requests one SD-to-W25Q256 sync and should clear the marker after successful D8CW and Aux4 sync/load.

V70_r2 HIL package rule:

- For formal V70_r2 board acceptance, prefer the V70_r2 playback package named by the AI deploy README:
  `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v70_wind_realistic_aux_public_single_r2\playback_hil\dataset_v70_r2_wind_realfield_e01sep_test_sd_g000000\wave`.
- If using the already verified `J:\wave`, record it explicitly as a compatibility/bring-up playback package, not as the formal V70_r2 matching playback package.

## 0.2 2026-06-21 V72/V73 Historical Alignment

Historical cross-project status after re-reading the AI and monitor handoff files on 2026-06-21:

- Monitor side: the currently deployed RC line is V72, tag `v72-single7-monitor-deployed-20260620-182036`. The monitor `network_generate_report.txt` points to `dataset_v72_wind_e00e01_separated_single7_final`, and `edgewind_ai.c` reports the V72 model version. V72 has build/flash/quick-read evidence, but remains an RC until golden-vector, normal 5-minute, and seven-class HIL gates are complete.
- AI training side: the latest AI candidate is V73, `dataset_v73_ad7606_sync_leakage_public_single`. Its final deploy package is `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v73_ad7606_sync_leakage_single7_final`, and its playback package is `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v73_ad7606_sync_leakage_public_single\playback_hil\dataset_v73_ad7606sync_test_sd_g000000\wave`.
- Playback side: no code change is required for V72 or V73 package format. The stable contract is still seven strict 4-channel D8CW `.bin` files plus `aux4_schedule.a4b`; the playback SD card still does not carry TFLite, X-CUBE-AI files, golden vectors, or AI self-test payloads.
- V73 monitor-side requirement from the AI handoff: AD7606 ch0..ch3 must remain strict simultaneous samples with no per-channel delay compensation, interpolation, or time shifting. Playback still outputs A/B/C/D as the D8CW package provides them; this is a monitor sampling/processing contract, not a playback format change.

2026-06-21 historical `J:\wave` status:

- At that time, the SD card package in `J:\wave` was V73: `dataset_v73_ad7606_sync_leakage_public_single`.
- Playback package validation passed: seven `.bin` files are `4194304` bytes, `sample_rate_hz=102400`, `channels=4`, `sample_count=516088`, `data_offset=64`, and all D8CW payload checksums match.
- `aux4_schedule.a4b` validation passed: `4512` bytes, `generation=1781955505`, `payload_fnv1a32=0xCC4AC198`, `dac_samples_per_aux=16384`, 224 total items, and all seven file entries bind to their matching D8CW checksum.
- Aux4 physical values stay within the configured ranges and map to E/F/G/H voltages inside `0.5V..4.5V`. Aggregate observed voltage range in this package is approximately `1.541V..3.304V` for E, `1.221V..2.289V` for F, `0.500V..4.500V` for G, and `1.371V..3.975V` for H.
- `SYNC_NOW.TXT` existed in `J:\wave`, so booting with that SD card requested one SD-to-W25Q256 sync.

Version-matching rule:

- For formal V72 HIL, use the AI V72 matching wave package, not the 2026-06-21 V73 `J:\wave`, unless the run is explicitly recorded as a compatibility/bring-up mismatch test.
- For formal V73 HIL, deploy the V73 monitor model first, then use the V73 package source path and re-check board logs.

## 0.3 2026-06-22 V74 Conservative + R_iso Current Line

Current cross-project status after re-reading the AI and monitor handoff files:

- Monitor side: the current board acceptance line is `dataset_v74_ad7606_sync_riso_public_single`. The monitor handoff says the firmware has been rolled back to the V74 conservative package, Keil rebuild passed, and flash/download completed with `Erase Done / Programming Done / Verify OK / Application running`.
- AI training side: the current deploy package is `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\stm32_deploy_packages\dataset_v74_ad7606_sync_riso_single7_conservative`.
- AI training side matching playback package for current HIL bring-up is `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v74_ad7606_sync_riso_public_single\playback_hil\dataset_v74_ad7606sync_riso_demo_sd_g000000\wave`. Older `dataset_v74_ad7606sync_riso_test_sd_g000000` packages must not be used as visual proof packages for the R_iso demo.
- Playback side: the D8CW+A4B file structure remains unchanged. Seven `.bin` files stay strict 4-channel A/B/C/D D8CW, and `aux4_schedule.a4b` remains the authoritative low-rate E/F/G/H sidecar.

V74 Aux4 semantic override:

```text
DAC8568 E -> T_igbt_C
DAC8568 F -> T_dc_cap_C
DAC8568 G -> RH_cabinet_pct
DAC8568 H -> R_iso_kohm
```

`wind_load_pct` is deprecated for the current V74 model and must not be fed into `X_aux[3]`. If old playback-side text mentions `wind_load_pct`, treat it as historical V69/V70/V72/V73 context only. Current V74 HIL must use `R_iso_kohm` for the fourth Aux4 value.

V74 monitor-side requirements that affect playback validation:

- AD7606 ch0..ch3 must remain strict simultaneous samples; monitor firmware must not add per-channel delay compensation, per-channel time shifting, or independent interpolation.
- AD7606 ch7 must decode the E/F/G/H sidecar's fourth value as engineering `R_iso_kohm`, not raw volts, raw ADC code, or load percentage.
- Board acceptance must still pass golden vectors, `normal.bin` 5 minutes without repeated E01/E04/E05, and seven-class HIL top1/confidence/timing recording before replacing the v68 fallback line.

## 0.4 2026-06-22 V74 R_iso Playback Fix And Board Evidence

Root cause of the Web value staying near `4000 kOhm`:

- The SD data was correct. The current V74 `aux4_schedule.a4b` uses `dataset_v74_ad7606_sync_riso_public_single`, `generation=1782121193`, `payload_fnv1a32=0x97CBCE3E`, and the fourth Aux4 range is `R_iso_kohm 20..8000`.
- The old playback firmware still treated the fourth Aux4 channel as legacy `wind_load_pct 8..110`, so it rejected the V74 header and kept outputting the legacy default H value as `2.5V`.
- The monitor correctly decoded that `2.5V` with the V74 `20..8000 kOhm` range, which gives about `4010 kOhm`. This matched the observed Web value and is not a DAC8568 hardware fault, monitor ADC fault, upload fault, or Web display fault.

Playback-side fix now implemented:

- `dac8568_aux4.c` accepts both fourth-channel contracts: legacy `8..110` and V74 `20..8000`; the first three Aux4 ranges remain fixed at `20..125`, `18..115`, and `8..98`.
- After a valid `.a4b` is loaded, playback copies `aux_lo[]`, `aux_hi[]`, and `aux_default[]` from the A4B header into runtime mapping state. DAC8568 E/F/G/H voltage conversion now uses those runtime ranges.
- Serial status now prints `range_lo_x1000`, `range_hi_x1000`, and `default_x1000` so field logs can prove whether H is running as legacy wind load or V74 R_iso.
- Boot now verifies the loaded Aux4 table against the currently ready seven D8CW checksums. If the QSPI Aux4 slot is stale or mismatched, firmware automatically tries transient load from `0:/wave/aux4_schedule.a4b` on SD and verifies again.

Board evidence after rebuild and download:

```text
Keil rebuild: 0 Error(s), 0 Warning(s)
Keil flash:   Erase Done. Programming Done. Verify OK. Application running ...
```

Observed reset log with the SD card currently inserted in the board:

```text
[AUX4] loaded source=1 generation=1781955505 checksum=0xCC4AC198 dataset=dataset_v73_ad7606_sync_leakage_public_single
[AUX4] bind summary: ready=7 verified=0 all=0
[AUX4] ready-wave bind not verified, trying SD transient load: 0:/wave/aux4_schedule.a4b
[AUX4] loaded source=2 generation=1782121193 checksum=0x97CBCE3E range_hi_x1000=[125000,115000,98000,8000000] default_x1000=[72500,66500,53000,400000] dataset=dataset_v74_ad7606_sync_riso_public_single
[AUX4] bind summary: ready=7 verified=7 all=1
[AUX4] loaded=1 default=0 source=2 gen=1782121193 checksum=0x97CBCE3E ... mv=[2079,1853,506,3215]
```

Interpretation:

- `source=1 generation=1781955505` is the old V73 Aux4 table still present in W25Q. It is rejected because its D8CW checksums do not match the current V74 D8CW partitions.
- `source=2 generation=1782121193` is the V74 A4B transiently loaded from SD. This is the active runtime source after the fix.
- `range_hi_x1000[3]=8000000` proves the H channel is now mapped as `R_iso_kohm 20..8000`, not legacy `wind_load_pct 8..110`.
- Normal-package H output is no longer fixed at `2500mV`; observed normal values are about `2.96V..3.25V`, matching `R_iso_kohm` around `4.9M..5.5M`.
- Because `SYNC_NOW.TXT` was absent, the W25Q Aux4 slot was not rewritten. Runtime is correct while the SD card is present. For no-SD boot or persistent W25Q Aux4, place `0:/wave/SYNC_NOW.TXT` on the SD card and reset once.

Programming note:

- Do not program this H750 target by dragging the hex file to the `DAPLINK` mass-storage drive. That DAPLink volume reports an F103-style board target and returns `Flash algorithm write verify command FAILURE`.
- Use the Keil target download path. In this run, waiting for the Keil flash command to finish produced the valid `Erase Done / Programming Done / Verify OK / Application running ...` log.

## 0.5 2026-07-01 Playback Output Stability Test

Current published rollback point:

- Tag `V3.1.0-v74-riso-20260701` points to commit `543550a` and is the V74 R_iso/Aux4 playback baseline.
- Optimization branch `codex/playback-output-stability-optimization` was tested on board after the baseline tag.

Board-tested result:

- The only retained output-stability change is `DAC8568_REF_REFRESH_MS = 0`, which disables the once-per-second in-stream DAC reference refresh frames.
- The current single-change firmware built with Keil `0 Error(s), 0 Warning(s)` and was downloaded through Keil. The user reported output is currently normal.

Do not repeat these rejected optimizations on the current DuPont-wire/DAC8568 module setup:

- Do not change SPI1 `PA5/SCK` and `PA7/MOSI` from `GPIO_SPEED_FREQ_LOW` to `GPIO_SPEED_FREQ_VERY_HIGH`. The high-speed GPIO setting caused DAC8568 communication errors and the analog output became chaotic.
- Do not change W25Q256 memory-mapped 0xEC read from `DummyCycles = 4` to `W25Qxx_DUMMY_CYCLES = 8`. The `8` dummy-cycle memory-map setting made A/B/C/D collapse into four straight lines.
- It is still acceptable for command-mode checked reads to use `W25Qxx_DUMMY_CYCLES = 8`; the rejected change is only for the QSPI memory-mapped playback path.

Current safe playback timing boundary:

- Keep SPI1 GPIO slew conservative: `PA5/PA7 = GPIO_SPEED_FREQ_LOW`, `PA4/SYNC = GPIO_SPEED_FREQ_VERY_HIGH`.
- Keep QSPI memory-mapped playback at `DummyCycles = 4`.
- Keep D8CW format, W25Q partitioning, Aux4 `.a4b`, and E/F/G/H injection unchanged.
- If any later optimization causes abnormal A-D output, immediately revert to `V3.1.0-v74-riso-20260701` before continuing tests.

## 1. Current Decision

V69 uses the existing high-speed D8CW waveform package plus a new Aux4 binary sidecar:

- A/B/C/D remain high-speed D8CW from seven `.bin` files.
- E/F/G/H are low-speed Aux4 outputs from `aux4_schedule.a4b`.
- The seven `.bin` files remain `channels=4`; do not expand D8CW to eight channels.
- `aux4_schedule.a4b` is the authoritative file for stable playback.
- `aux4_schedule.json` may be generated as a debug mirror only.

One Aux4 item corresponds to:

```text
4096 monitor ADC samples @ 25.6 kHz = 160 ms
16384 playback DAC samples @ 102.4 kHz = 160 ms
```

## 2. AI Training Side: SD Card Package

AI must generate this SD card layout:

```text
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/igbt_fault.bin
0:/wave/bus_ground.bin
0:/wave/pwm_abnormal.bin
0:/wave/aux4_schedule.a4b
0:/wave/summary.json
0:/wave/SYNC_NOW.TXT
```

Optional debug file:

```text
0:/wave/aux4_schedule.json
```

The seven D8CW `.bin` files must keep:

```text
channels = 4
sample_rate_hz = 102400
order = normal, ac_coupling, insulation, cap_aging, igbt_fault, bus_ground, pwm_abnormal
```

AI must read or produce each D8CW header and use these fields when building `aux4_schedule.a4b`:

```text
d8cw_sample_rate_hz
d8cw_sample_count
d8cw_checksum
```

For each `.bin`:

```text
item_count = ceil(d8cw_sample_count / 16384)
```

Each Aux4 item is four little-endian float32 values in this fixed order:

```text
T_igbt_C
T_dc_cap_C
RH_cabinet_pct
wind_load_pct
```

Physical clamp ranges:

```text
T_igbt_C          20..125
T_dc_cap_C        18..115
RH_cabinet_pct     8..98
wind_load_pct      8..110
```

## 3. Aux4 Binary Format

All fields are little-endian.

Header:

```text
size: 256 bytes
magic: "EWAUX4\0\0"
version: 1
dac_sample_rate_hz: 102400
monitor_sample_rate_hz: 25600
monitor_window_points: 4096
dac_samples_per_aux: 16384
aux_count: 4
file_count: 7
file_entry_bytes: 96
item_stride_bytes: 16
payload_fnv1a32: FNV-1a32 over [file_table_offset, total_bytes)
generation: package build timestamp or monotonic increasing id
```

File entry:

```text
size: 96 bytes
bin_name: one of the seven canonical .bin names
partition_id: 0..6, same as D8CW partition
class_id: 0..6, must equal partition_id
d8cw_sample_rate_hz: from matching D8CW header
d8cw_sample_count: from matching D8CW header
d8cw_checksum: from matching D8CW header
item_start: first Aux4 item index
item_count: ceil(d8cw_sample_count / 16384)
samples_per_item: 16384
flags: 0
records_fnv1a32: FNV-1a32 over this file's item block
```

Item:

```text
size: 16 bytes
float32 value[4]
```

`summary.json` must record at least:

```text
aux4_schedule.a4b generation
aux4_schedule.a4b payload_fnv1a32
per-file d8cw_sample_count
per-file d8cw_checksum
per-file item_count
```

## 4. Playback Side Behavior

W25Q256 layout:

```text
0x00000000..0x0000FFFF  Aux4 slot0, 64KB
0x00010000..0x0001FFFF  Aux4 slot1, 64KB
0x00400000..0x01FFFFFF  seven D8CW 4MB partitions, unchanged
```

Boot behavior:

- If `0:/wave/SYNC_NOW.TXT` exists, playback firmware syncs seven D8CW `.bin` files, then syncs `aux4_schedule.a4b` into the inactive Aux4 slot.
- After D8CW sync and Aux4 sync/load are complete, firmware deletes `SYNC_NOW.TXT`.
- If no sync marker exists, firmware loads the newest valid Aux4 slot by `generation`.
- If both Aux4 slots are invalid, firmware may try SD transient load.
- If Aux4 is missing or invalid, A/B/C/D playback still runs; E/F/G/H output default midpoints.

Aux4 binding is strict. For every partition, playback checks:

```text
sample_rate_hz matches D8CW header
sample_count matches D8CW header
checksum matches D8CW header
item_count == ceil(sample_count / 16384)
```

If a partition fails binding, that partition uses default Aux4 values.

DAC output mapping:

```text
analog_V = 0.5 + 4.0 * (clamp(value, lo, hi) - lo) / (hi - lo)
```

Runtime scheduling:

- Aux4 item index is derived from the current D8CW QSPI playback sample index.
- Fault trigger switches to the matching Aux4 file and starts from item 0.
- Fault stop returns to normal Aux4 using the current normal D8CW sample index.
- E/F/G/H frames are injected at DMA refill boundary; the high-speed A/B/C/D format remains four-channel D8CW.

## 5. Monitor Side Sampling

Required wiring:

```text
DAC8568 A -> AD7606 ch0
DAC8568 B -> AD7606 ch1
DAC8568 C -> AD7606 ch2
DAC8568 D -> AD7606 ch3
DAC8568 E -> AD7606 ch4
DAC8568 F -> AD7606 ch5
DAC8568 G -> AD7606 ch6
DAC8568 H -> AD7606 ch7
GND common
```

Monitor sampling contract:

- AD7606 continues sampling 8 raw channels at `25600 Hz`.
- ch0-ch3 keep the existing high-frequency AI window path.
- ch4-ch7 are low-speed held voltages from E/F/G/H.
- For each `4096` point AI window, monitor computes the mean of ch4, ch5, ch6, ch7.
- The four means are decoded from `0.5..4.5V` back to physical Aux4 values.

Decode formula:

```text
value = lo + (clamp(analog_V, 0.5, 4.5) - 0.5) * (hi - lo) / 4.0
```

V69 logical model tensors:

```text
X_dwt[104]
X_feat[116]
X_spec[512,4]
X_aux[4]
```

Current V69 publicfix RC X-CUBE-AI STM32 generated input order:

```text
input[0] = X_aux[4]
input[1] = X_dwt[104]
input[2] = X_feat[116]
input[3] = X_spec[512,4]
```

Do not bind monitor inputs by an assumed TFLite or conceptual order. The monitor firmware must bind by the generated X-CUBE-AI input list/names in `network_generate_report.txt`. The AI-side deploy README records that the observed TFLite order differs from the STM32 generated order.

`X_aux[4]` order:

```text
X_aux[0] = T_igbt_C
X_aux[1] = T_dc_cap_C
X_aux[2] = RH_cabinet_pct
X_aux[3] = wind_load_pct
```

If a 4096-point window crosses an Aux4 update boundary, the mean may mix two adjacent items. This is acceptable and should be covered by AI training augmentation with noise, dropout, and boundary mixing.

Monitor validity fallback:

- If a ch4-ch7 window mean is below `0.25V` or above `4.75V`, the monitor uses the default physical value for that Aux4 channel and clears the corresponding `aux_valid` bit.
- Default physical values are `72.5, 66.5, 53.0, 59.0` for `T_igbt_C, T_dc_cap_C, RH_cabinet_pct, wind_load_pct`.
- Timing pressure must not slow ch0-ch3 acquisition; holding the previous Aux4 value is preferable to disturbing the high-speed window.

## 6. Acceptance Notes

Playback-side serial logs should show:

```text
[AUX4] sync ok ...
[AUX4] qspi load ok ...
[AUX4] bind ok ...
[AUX4] loaded=... source=... gen=... checksum=... file=... items=... item=... inject=...
```

Monitor-side serial logs should show physical Aux4 values and V69 inference output:

```text
model_version=dataset_v69_wind_sensor_aux4_public_fused_single_publicfix
aux4=[T_igbt_C,T_dc_cap_C,RH_cabinet_pct,wind_load_pct]
aux_valid=0x...
ppermil=[...] feature_ms=... infer_ms=... total_ms=...
```

Regression rule:

- Missing or corrupt Aux4 must not stop A/B/C/D D8CW playback.
- E/F/G/H must remain within `0.5..4.5V`.
- Normal should remain stable as E00 before fault validation.
