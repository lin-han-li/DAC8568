Copy the `wave/` folder in this directory to the SD card root.

Expected on target:
0:/wave/normal.bin
0:/wave/ac_coupling.bin
0:/wave/bus_ground.bin
0:/wave/insulation.bin
0:/wave/cap_aging.bin
0:/wave/pwm_abnormal.bin
0:/wave/igbt_fault.bin

Regenerate:
python tools/gen_dac_fault_suite.py --out-dir sd_card_payload/copy_to_sd/wave
