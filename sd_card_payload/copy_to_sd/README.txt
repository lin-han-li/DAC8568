Copy the `wave/` folder in this directory to the SD card root.
For V69 AI HIL playback packages, keep `wave/aux4_schedule.a4b`,
`wave/summary.json`, and `wave/SYNC_NOW.TXT` with the 7 waveform files.

Expected on target:
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

Canonical cross-project order:
E00 normal
E01 ac_coupling
E02 insulation
E03 cap_aging
E04 igbt_fault
E05 bus_ground
E06 pwm_abnormal

Regenerate:
python tools/gen_dac_fault_suite.py --out-dir sd_card_payload/copy_to_sd/wave

Sync trigger:
- `SYNC_NOW.TXT` is a one-shot marker. If it exists on SD at boot, firmware syncs all 7 wave files into W25Q256.
- After all 7 partitions sync successfully, firmware syncs `aux4_schedule.a4b` to W25Q256 Aux4 dual-slot storage.
- Firmware deletes `SYNC_NOW.TXT` after D8CW sync and Aux4 sync/load are complete.
- If this marker is absent, firmware skips SD sync and uses existing valid W25Q256 wave data.

Scale convention:
- DAC8568 output is a low-voltage analog signal in the -5V..+5V range.
- A/B bus channels use -5V..+5V analog as -500V..+500V engineering full scale.
- Normal bus is about A=+3.0V / B=-3.0V, equivalent to about +300V / -300V.
- AI training uses the same low-voltage analog value expressed in mV, so the normal bus is +3000mV / -3000mV.

Current generated file layout:
- File size: 4194304 bytes (4MB)
- sample_rate_hz: 102400
- sample_count: 516088
- data_bytes: 4128704
- Tail guard: 65536 bytes reserved at the end of each 4MB W25Q256 partition

V69 Aux4:
- `aux4_schedule.a4b` is required for V69 Aux4 stable playback; `aux4_schedule.json` is optional debug output only.
- One Aux4 item covers 16384 DAC samples @ 102400Hz, equal to one 4096-point monitor window @ 25600Hz.
- Per D8CW file, `item_count = ceil(d8cw_sample_count / 16384)`.
- Item value order is `T_igbt_C`, `T_dc_cap_C`, `RH_cabinet_pct`, `wind_load_pct`.
- Playback maps each value to 0.5V..4.5V on DAC8568 E/F/G/H.

V70_r2/V72/V73/V74:
- SD format is unchanged: 7 strict 4-channel D8CW `.bin` files plus `aux4_schedule.a4b`, `summary.json`, and `SYNC_NOW.TXT`.
- Do not make 8-channel D8CW files. E/F/G/H Aux4 values still come from `aux4_schedule.a4b`.
- `aux4_schedule.json` is still debug-only.
- Do not copy TFLite, X-CUBE-AI network files, golden vectors, or AI self-test payloads to the playback SD card.
- For formal HIL, use the matching AI-generated `wave/` package for the monitor model version under test and re-check it before syncing.

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

Fault model notes:
- ac_coupling: 50Hz common-mode injection and leakage rise.
- insulation: leakage drift with partial-discharge pulses.
- cap_aging: high DC-link ripple and ESR recharge spikes.
- igbt_fault: short-circuit spike, bus sag and protection recovery.
- bus_ground: bus collapse, current surge and leakage spike.
- pwm_abnormal: carrier jitter, missing pulses and current modulation.
