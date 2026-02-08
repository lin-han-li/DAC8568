Copy these files to your SD card root:

- sd_card_payload/wave/dac8568_wave.bin -> SD:/wave/dac8568_wave.bin

Firmware behavior:

- If SD sync to W25Q256 succeeds: waveform streams from QSPI memory-mapped region.
- If SD sync fails: waveform output is disabled (fail-closed).

Wave file format:

- Uses legacy D8CW header format (same as commit 504cb0...).
- Generator: `python tools/gen_dac_sd_wave.py`
