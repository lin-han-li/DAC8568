Copy the folder "wave" to SD card root.

Expected file path on target:
0:/wave/dac8568_wave.bin

Boot behavior:
1) Firmware tries SD -> QSPI sync from the file above.
2) If sync succeeds, DAC switches to QSPI direct-read playback.
3) If SD file is absent, firmware tries loading waveform header from QSPI.
4) If both fail, firmware falls back to built-in LUT waveform.
