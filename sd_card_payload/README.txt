Copy `sd_card_payload/copy_to_sd/wave/` to SD card root as `wave/`.

Expected file paths on target:
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/bus_ground.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/pwm_abnormal.bin
0:/wave/igbt_fault.bin

Generate test files:
python tools/gen_dac_fault_suite.py --out-dir sd_card_payload/copy_to_sd/wave

Boot behavior (RTOS started):
1) Firmware syncs ALL 7 files from SD -> W25Q256 partitions (takes time).
2) After baseline partition is ready, DAC switches to QSPI direct-read playback and loops forever.
3) Fault trigger is pointer switching only (no SD read / no QSPI erase/write during trigger).
4) If a fault partition isn't ready, UI disables trigger for that fault.

Notes:
- Baseline output requires SD sync by default (see `DAC_WAVE_REQUIRE_SD_SYNC` in `Core/Inc/main.h`).
- If baseline isn't ready, firmware outputs no waveform (stream disabled).

Scale convention:
- DAC8568 output is a low-voltage analog signal in the -5V..+5V range.
- A/B bus channels use -5V..+5V analog as -500V..+500V engineering full scale.
- 1V analog = 100V bus engineering value.
- Normal bus is not full scale: A ~ +3.0V means about +300V, B ~ -3.0V means about -300V.
- AI training uses the same low-voltage analog value expressed in mV, so +3.0V/-3.0V corresponds to +3000mV/-3000mV.

Channel semantics (DAC/ADC low-voltage analog domain):
- A: DC bus positive voltage (bipolar, normal positive, +3V analog ~= +300V engineering)
- B: DC bus negative voltage (bipolar, normal negative, -3V analog ~= -300V engineering)
- C: Load current (unipolar, >=0)
- D: Leakage current (unipolar, >=0)

Baseline waveform (normal.bin):
- A ~ +3.0V, B ~ -3.0V with small 100Hz ripple
- C ~ +1.5V (unipolar) with small 100Hz ripple
- D ~ +0.05V (unipolar) with small noise

Fault waveforms:
- Each fault bin is generated as an independent waveform (not "baseline + overlay"),
  so Simulink-exported fault bins can directly replace `wave/*.bin` if they follow the same header+data format.
- ac_coupling: 50Hz common-mode injection, zero-cross switching noise and leakage rise.
- bus_ground: periodic bus collapse, over-current surge, leakage spike and damped recovery.
- insulation: slow insulation drift, elevated leakage and partial-discharge pulse bursts.
- cap_aging: higher 100/120Hz DC-link ripple, deeper valleys and ESR recharge spikes.
- pwm_abnormal: carrier ripple, duty jitter, missing-pulse dropout and current modulation.
- igbt_fault: desaturation/short-circuit spike, bus sag, protection clamp and recovery.

Default sample rate in generated files:
- 102400 Hz

Current generated file layout:
- File size: 4194304 bytes (4MB)
- Header size: 64 bytes
- Playable sample_count: 516088 samples
- Data bytes covered by checksum: 4128704 bytes
- Tail guard: 65536 bytes reserved at the end of each 4MB W25Q256 partition
