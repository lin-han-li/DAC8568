Copy `sd_card_payload/copy_to_sd/wave/` to SD card root as `wave/`.
For V69 AI HIL playback packages, copy the whole `wave/` folder, including
`aux4_schedule.a4b`, `summary.json`, and `SYNC_NOW.TXT`.

Expected file paths on target:
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/igbt_fault.bin
0:/wave/bus_ground.bin
0:/wave/pwm_abnormal.bin
0:/wave/aux4_schedule.a4b (V69 Aux4 authoritative sidecar)
0:/wave/summary.json        (optional HIL traceability manifest)
0:/wave/SYNC_NOW.TXT        (one-shot SD->W25Q256 sync trigger)

Canonical cross-project order:
E00 normal
E01 ac_coupling
E02 insulation
E03 cap_aging
E04 igbt_fault
E05 bus_ground
E06 pwm_abnormal

Generate test files:
python tools/gen_dac_fault_suite.py --out-dir sd_card_payload/copy_to_sd/wave

Boot behavior (RTOS started):
1) Firmware checks `0:/wave/SYNC_NOW.TXT`.
2) If the flag file exists, firmware syncs all 7 files from SD -> W25Q256 partitions, then syncs `aux4_schedule.a4b` to the W25Q Aux4 dual-slot area.
3) Firmware deletes the flag after D8CW sync and Aux4 sync/load are complete.
4) If the flag file does not exist, firmware skips SD sync and loads existing valid wave metadata from W25Q256.
5) After D8CW partitions are ready, firmware verifies Aux4 against the current D8CW checksums. If the W25Q Aux4 slot is stale, firmware transiently loads `0:/wave/aux4_schedule.a4b` from SD and verifies again.
6) After baseline partition is ready, DAC switches to QSPI direct-read playback and loops forever.
7) Fault trigger is pointer switching only (no SD read / no QSPI erase/write during trigger).
8) If a fault partition isn't ready, UI disables trigger for that fault.

Notes:
- To force a new SD->W25Q256 update, create `wave/SYNC_NOW.TXT` on the SD card before inserting it into the board and pressing reset.
- Baseline output does not require SD sync by default (see `DAC_WAVE_REQUIRE_SD_SYNC` in `Core/Inc/main.h`).
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
- E: T_igbt_C, V69 Aux4, 0.5V..4.5V maps to 20..125 degC
- F: T_dc_cap_C, V69 Aux4, 0.5V..4.5V maps to 18..115 degC
- G: RH_cabinet_pct, V69 Aux4, 0.5V..4.5V maps to 8..98 %
- H: R_iso_kohm for the current V74 Aux4 contract. Older V69/V70/V72/V73 packages used wind_load_pct here.

V69 Aux4 sidecar:
- `aux4_schedule.a4b` is the stable playback file; optional `aux4_schedule.json` is debug-only.
- One Aux4 item covers 16384 DAC samples @ 102400Hz, equal to one 4096-point monitor window @ 25600Hz.
- Per file, `item_count = ceil(d8cw_sample_count / 16384)`.
- The sidecar is synced to W25Q256 slot0/slot1 at 0x00000000/0x00010000 and is not written into the seven 4MB D8CW partitions.
- If Aux4 is missing or corrupt, A-D playback still runs and E-H output default midpoint values.

V70_r2/V72/V73/V74 note:
- V70_r2, V72, V73, and V74 keep the same playback SD format: 7 strict 4-channel D8CW `.bin` files, `aux4_schedule.a4b`, `summary.json`, and `SYNC_NOW.TXT`.
- Do not convert D8CW to 8 channels. E/F/G/H still come only from `aux4_schedule.a4b`.
- Do not place TFLite, X-CUBE-AI network files, golden vectors, or AI self-test payloads on the playback SD card.
- `aux4_schedule.json` remains a debug mirror only, not the stable playback input.
- For formal model HIL, use the matching AI-generated `wave/` package for the monitor model version under test and re-run package validation before SD-to-W25Q sync.

V74 current contract:
- Current AI/monitor candidate: `dataset_v74_ad7606_sync_riso_public_single`.
- Matching playback wave package:
  `C:\Users\pengjianzhong\Desktop\MY_Project\EdgeWind_AI_Training\data_v74_ad7606_sync_riso_public_single\playback_hil\dataset_v74_ad7606sync_riso_demo_sd_g000000\wave`
- E/F/G/H value order is `T_igbt_C`, `T_dc_cap_C`, `RH_cabinet_pct`, `R_iso_kohm`.
- `wind_load_pct` is deprecated and must not be used as the fourth Aux4 model input for V74.

2026-06-22 checked board SD package:
- The SD card currently inserted in the playback board contains the V74 package `dataset_v74_ad7606_sync_riso_public_single`.
- Seven D8CW files are 4194304 bytes, 102400 Hz, channels=4, sample_count=516088, and all payload checksums match the V74 package.
- `aux4_schedule.a4b` is 4512 bytes, generation 1782121193, payload_fnv1a32 0x97CBCE3E, with H mapped as `R_iso_kohm 20..8000`.
- Reset log evidence: old W25Q Aux4 generation 1781955505 failed checksum binding, then SD transient V74 Aux4 loaded successfully with `bind summary: ready=7 verified=7 all=1`.
- Current runtime is correct with SD present. For persistent no-SD Aux4, create `wave/SYNC_NOW.TXT` and reset once to rewrite the W25Q Aux4 dual slot.

Baseline waveform (normal.bin):
- A ~ +3.0V, B ~ -3.0V with small 100Hz ripple
- C ~ +1.5V (unipolar) with small 100Hz ripple
- D ~ +0.05V (unipolar) with small noise

Fault waveforms:
- Each fault bin is generated as an independent waveform (not "baseline + overlay"),
  so Simulink-exported fault bins can directly replace `wave/*.bin` if they follow the same header+data format.
- ac_coupling: 50Hz common-mode injection, zero-cross switching noise and leakage rise.
- insulation: slow insulation drift, elevated leakage and partial-discharge pulse bursts.
- cap_aging: higher 100/120Hz DC-link ripple, deeper valleys and ESR recharge spikes.
- igbt_fault: desaturation/short-circuit spike, bus sag, protection clamp and recovery.
- bus_ground: periodic bus collapse, over-current surge, leakage spike and damped recovery.
- pwm_abnormal: carrier ripple, duty jitter, missing-pulse dropout and current modulation.

Default sample rate in generated files:
- 102400 Hz

Current generated file layout:
- File size: 4194304 bytes (4MB)
- Header size: 64 bytes
- Playable sample_count: 516088 samples
- Data bytes covered by checksum: 4128704 bytes
- Tail guard: 65536 bytes reserved at the end of each 4MB W25Q256 partition
