# Playback Expansion Board Pinout

Date: 2026-06-23

This note records the small playback-side expansion board only for the parts that are currently connected by DuPont wires:

- DAC8568 module adapter
- Four keys
- Rotary encoder with push switch
- Buzzer

Core-board resources are not duplicated here: SD, W25Q/QSPI, UART, SWD, LCD, SDRAM, SDMMC, and LTDC stay on the existing core board.

Checked sources:

- `Core/Inc/main.h`
- `Core/Src/gpio.c`
- `Core/Src/spi.c`
- `Core/Src/tim.c`

## Required Signals

| Function | MCU pin | Code name | Firmware role | Expansion-board connection |
|---|---:|---|---|---|
| DAC SYNC/CS | PA4 | `DAC8568_SYNC_Pin` | `SPI1_NSS`, AF5, hardware NSS | DAC8568 `SYNC/CS`, active low |
| DAC SCLK | PA5 | `DAC85_CLK_Pin` | `SPI1_SCK`, AF5 | DAC8568 `SCLK` |
| DAC DIN | PA7 | `DAC8568_DIN_Pin` | `SPI1_MOSI`, AF5 | DAC8568 `DIN/MOSI` |
| DAC LDAC | PA6 | `DAC8568_LDAC_Pin` | GPIO output | DAC8568 `LDAC` |
| DAC CLR | PB9 | `DAC8568_CLR_Pin` | GPIO output | DAC8568 `CLR` |
| Key 1 / Previous | PB12 | `KEY1_Pin` | EXTI falling, pull-up, `LV_KEY_PREV` | Switch to GND |
| Key 2 / Next | PB13 | `KEY2_Pin` | EXTI falling, pull-up, `LV_KEY_NEXT` | Switch to GND |
| Key 3 / OK | PB14 | `KEY3_Pin` | EXTI falling, pull-up, `LV_KEY_ENTER` | Switch to GND; use for encoder push switch if OK is needed |
| Key 4 / Back | PB15 | `KEY4_Pin` | EXTI falling, pull-up, `LV_KEY_ESC` | Switch to GND |
| Encoder A | PC6 | TIM8 CH1 | Encoder input, AF3, no pull | Encoder A phase |
| Encoder B | PC7 | TIM8 CH2 | Encoder input, AF3, no pull | Encoder B phase |
| Buzzer control | PH7 | `BEEF_Pin` | GPIO output, boot high | Active-low buzzer control, high = off |

Power and reference:

- Route a reliable common `GND` between core board, expansion board, DAC8568 module, and monitor-side analog ground.
- Route `3V3` for logic pull-ups and modules that need 3.3 V logic.
- If the current DAC8568 module needs `5V`, `AVDD`, or an external reference supply, pass that supply through according to the existing module wiring.

## DAC8568 Module Adapter

The expansion board should not place the DAC8568 chip itself in this version. It only adapts the existing DAC8568 module.

Connections:

| MCU pin | DAC8568 module pin | Notes |
|---:|---|---|
| PA4 | `SYNC` / `CS` | SPI chip select, active low |
| PA5 | `SCLK` | SPI clock |
| PA7 | `DIN` / `MOSI` | SPI data input |
| PA6 | `LDAC` | Must not float; firmware drives it during playback |
| PB9 | `CLR` | Default high in firmware |
| GND | `GND` | Common ground |
| 3V3 | Logic supply if required | Match current module wiring |
| Optional 5V/AVDD/VREF | Module supply/reference | Only if the current module uses it |

Design notes:

- Do not route `MISO`; current SPI1 is TX-only.
- SPI1 is configured as 32-bit transmit, hardware NSS, prescaler 4. Treat `SCK`, `MOSI`, and `SYNC` as fast digital lines, roughly tens of MHz.
- Keep `SCK`, `MOSI`, and `SYNC` short, with adjacent ground where possible.
- Reserve optional `22R` to `47R` series resistor footprints on `SCK`, `MOSI`, and `SYNC`.
- Reserve a weak pull-up footprint on `CLR` to `3V3`; `10k` is suitable.
- `LDAC` must be routed and held to the module. Do not leave it floating.
- Preserve the current DAC8568 module A-H analog outputs. They continue to feed the monitor-side AD7606 wiring.

## Keys

The four key inputs are active low.

| Key | MCU pin | Connection |
|---|---:|---|
| `KEY1` / Previous | PB12 | GPIO to switch, switch to GND |
| `KEY2` / Next | PB13 | GPIO to switch, switch to GND |
| `KEY3` / OK | PB14 | GPIO to switch, switch to GND |
| `KEY4` / Back | PB15 | GPIO to switch, switch to GND |

Design notes:

- Firmware enables internal pull-ups and falling-edge EXTI on PB12-PB15.
- Current LVGL key mapping is defined in `lvgl-9.4.0/examples/porting/lv_port_indev.c`: PB12 = previous, PB13 = next, PB14 = enter/OK, PB15 = ESC/back.
- Add optional `100nF` capacitor footprints from each key signal to GND for hardware debounce. Mark them DNP by default if fast key edges are preferred.
- No confirmed current business-level `HAL_GPIO_EXTI_Callback` handling was found. The board should route the keys now; firmware behavior can be added later.

## Rotary Encoder

| Encoder signal | MCU pin | Connection |
|---|---:|---|
| A phase | PC6 | TIM8 CH1 |
| B phase | PC7 | TIM8 CH2 |
| Common | GND | Encoder common |
| Push switch | PB14 | Same net as `KEY3` / `OK` |

Design notes:

- PC6 and PC7 are configured as TIM8 encoder inputs with `GPIO_NOPULL`.
- Add `10k` pull-ups from PC6 and PC7 to `3V3` on the expansion board.
- Reserve optional `100nF` capacitor footprints from PC6 and PC7 to GND for mechanical encoder debounce or noise filtering.
- The encoder push switch should share PB14 / `KEY3` / `OK` if the push action is intended to be confirm.
- TIM8 encoder mode is initialized in firmware, but no confirmed `HAL_TIM_Encoder_Start()` call was found in the current active path. Hardware should be ready for later firmware enablement.

## Buzzer

| MCU pin | Code name | Required polarity |
|---:|---|---|
| PH7 | `BEEF_Pin` | High = off, low = on |

Design notes:

- Firmware initializes PH7 high at boot. The hardware must therefore be quiet when PH7 is high.
- Do not drive a high-current buzzer directly from the MCU pin.
- Preferred option: use a 3.3 V active buzzer module with active-low enable, or build an active-low transistor/MOSFET driver that is off when PH7 is high.
- If a future board uses an active-high low-side NMOS/NPN buzzer driver, the firmware boot default and drive polarity must be changed at the same time, otherwise the buzzer may turn on at boot.

## Explicitly Not Routed

Do not add these to this small expansion board:

- PH6 / `BG`: initialized high in GPIO, but it is an old LCD/backlight-related signal, not the buzzer.
- PG12 / `DC`, SPI6, LTDC, LCD, SDRAM, SDMMC, QSPI/W25Q, USART1, SWD.
- PD5 / PD6: initialized as inputs, but no clear current business role was found.

## Bring-Up Checklist

1. With power off, verify continuity for all required nets and verify no short between `3V3` and GND.
2. Power on and confirm the buzzer is silent with PH7 high.
3. Confirm PB12-PB15 are high when released and low when pressed.
4. Rotate the encoder and confirm PC6/PC7 show clean A/B phase transitions; confirm the push switch pulls PB12 low.
5. During playback, probe PA5, PA7, and PA4 to confirm SPI clock/data/SYNC activity.
6. Confirm DAC8568 A-H analog outputs recover the existing playback behavior.
7. Confirm common ground between playback board, DAC8568 module, and monitor-side AD7606.
