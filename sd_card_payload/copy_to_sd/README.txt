Copy the `wave/` folder in this directory to the SD card root.
For AI HIL playback packages, keep `wave/summary.json` and `wave/SYNC_NOW.TXT`
with the 7 waveform files so the injected class/subtype can be traced later.

Expected on target:
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/igbt_fault.bin
0:/wave/bus_ground.bin
0:/wave/pwm_abnormal.bin
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
- After all 7 partitions sync successfully, firmware deletes `SYNC_NOW.TXT`.
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

Fault model notes:
- ac_coupling: 50Hz common-mode injection and leakage rise.
- insulation: leakage drift with partial-discharge pulses.
- cap_aging: high DC-link ripple and ESR recharge spikes.
- igbt_fault: short-circuit spike, bus sag and protection recovery.
- bus_ground: bus collapse, current surge and leakage spike.
- pwm_abnormal: carrier jitter, missing pulses and current modulation.
