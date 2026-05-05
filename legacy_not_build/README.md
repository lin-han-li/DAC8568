# legacy_not_build

This directory keeps legacy source that is intentionally excluded from the
current Keil main build.

Current demo mainline:

- UI: `MDK-ARM/HARDWORK/EdgeWind_UI`
- Wave import: `MDK-ARM/HARDWORK/SD_Card/sd_waveform.c`
- QSPI usage: W25Q256 is reserved for DAC waveform partitions and playback.

Do not add these legacy files back to `MDK-ARM/STM32H750XBH6.uvprojx` without a
new QSPI partition and arbitration design. The old GUI-Guider resource sync and
old `dac_wave_sync` code can erase or remap QSPI while DAC8568 playback is using
memory-mapped waveform data.

Moved here:

- `GUI-Guider_Runtime`
- `GUI-Guider_Source`
- `old_dac_wave_sync`
