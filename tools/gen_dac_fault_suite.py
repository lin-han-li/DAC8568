#!/usr/bin/env python3
"""
Generate 7 x 4MB DAC8568 waveform binaries for SD -> W25Q256 full-sync.

Target SD paths on MCU:
  0:/wave/normal.bin
  0:/wave/ac_coupling.bin
  0:/wave/bus_ground.bin
  0:/wave/insulation.bin
  0:/wave/cap_aging.bin
  0:/wave/pwm_abnormal.bin
  0:/wave/igbt_fault.bin

Binary format matches MDK-ARM/HARDWORK/SD_Card/sd_waveform.h (SD_DacWaveHeader_t).
"""

from __future__ import annotations

import argparse
import math
import os
import struct
from dataclasses import dataclass
from typing import Callable, Iterable, Tuple


MAGIC = 0x44384357  # "D8CW"
VERSION = 1
CHANNELS = 4
DATA_OFFSET = 64

# The existing firmware code maps +/-5V "external" to DAC input around 1.25..3.75V with a 2.5V ref.
VREF_MV = 2500.0
VOUT_MAX = 5.0
VOUT_MIN = -5.0

PARTITION_BYTES = 4 * 1024 * 1024
FULL_SAMPLE_COUNT = (PARTITION_BYTES - DATA_OFFSET) // (CHANNELS * 2)  # 4ch x uint16


def voltage_to_code(voltage: float) -> int:
    clamped = max(VOUT_MIN, min(VOUT_MAX, voltage))
    real_voltage = clamped / 4.0 + 2.5
    scaled = (real_voltage / 2.0) * 1000.0 / VREF_MV
    code = int(scaled * 65535.0 + 0.5)
    if code < 0:
        return 0
    if code > 65535:
        return 65535
    return code


def checksum_update(value: int, data: bytes) -> int:
    # Same as firmware: value = (value * 16777619u) ^ byte
    for b in data:
        value = ((value * 16777619) ^ b) & 0xFFFFFFFF
    return value


class XorShift32:
    def __init__(self, seed: int = 0x12345678) -> None:
        self._x = seed & 0xFFFFFFFF

    def next_u32(self) -> int:
        x = self._x
        x ^= (x << 13) & 0xFFFFFFFF
        x ^= (x >> 17) & 0xFFFFFFFF
        x ^= (x << 5) & 0xFFFFFFFF
        self._x = x & 0xFFFFFFFF
        return self._x

    def next_float(self) -> float:
        # [0, 1)
        return (self.next_u32() & 0xFFFFFF) / float(1 << 24)

    def noise(self, amp: float) -> float:
        return (self.next_float() * 2.0 - 1.0) * amp


def clamp(v: float, lo: float = VOUT_MIN, hi: float = VOUT_MAX) -> float:
    return max(lo, min(hi, v))


@dataclass(frozen=True)
class WaveSpec:
    name: str
    filename: str
    func: Callable[[int, float, XorShift32], Tuple[float, float, float, float]]


def make_base_cycles(sample_rate: int, sample_count: int) -> Tuple[int, int, int, int]:
    # Choose integer cycles per file so wave matches at wrap-around.
    # approx A=1k, B=2k, C=3k, D=4k for sample_rate=240k, sample_count=524280
    return (2184, 4369, 6553, 8738)


def base_wave(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    ca, cb, cc, cd = make_base_cycles(sample_rate, sample_count)
    amp = 4.0
    pa = 2.0 * math.pi * ca * (i / sample_count)
    pb = 2.0 * math.pi * cb * (i / sample_count)
    pc = 2.0 * math.pi * cc * (i / sample_count)
    pd = 2.0 * math.pi * cd * (i / sample_count)
    return (
        amp * math.sin(pa),
        amp * math.sin(pb),
        amp * math.sin(pc),
        amp * math.sin(pd),
    )


def wave_normal(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    # Baseline should be DC on all 4 channels.
    LV_DC = 0.0
    return (LV_DC, LV_DC, LV_DC, LV_DC)


def wave_ac_coupling(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    a, b, c, d = base_wave(i, t, rng, sample_rate, sample_count)
    cm = 0.8 * math.sin(2.0 * math.pi * 50.0 * t) + 0.35 * math.sin(2.0 * math.pi * 100.0 * t)
    hf = 0.15 * math.sin(2.0 * math.pi * 8000.0 * t)
    return tuple(clamp(v + cm + hf) for v in (a, b, c, d))  # type: ignore[return-value]


def wave_bus_ground(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    a, b, c, d = base_wave(i, t, rng, sample_rate, sample_count)
    # DC offset drift + partial saturation segments
    drift = 1.8 * math.sin(2.0 * math.pi * 0.8 * t)
    step = 2.0 if (i % 12000) < 6000 else -1.0  # ~50ms step segments at 240k
    v = [a * 0.9 + drift + step, b * 0.7 + drift + step, c * 0.6 + drift + step, d * 0.8 + drift + step]
    # Add a rare deep glitch on channel C
    if (i % 48000) == 1200:
        v[2] -= 4.5
    return tuple(clamp(x) for x in v)  # type: ignore[return-value]


def wave_insulation(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    a, b, c, d = base_wave(i, t, rng, sample_rate, sample_count)
    slow = 0.9 * math.sin(2.0 * math.pi * 0.3 * t) + 0.4 * math.sin(2.0 * math.pi * 0.05 * t)
    n = rng.noise(0.25)
    v = [a * 0.95 + slow + n, b * 0.9 + slow + rng.noise(0.20), c * 0.85 + slow + rng.noise(0.20), d * 0.9 + slow + rng.noise(0.20)]
    return tuple(clamp(x) for x in v)  # type: ignore[return-value]


def wave_cap_aging(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    a, b, c, d = base_wave(i, t, rng, sample_rate, sample_count)
    ripple = 0.9 * math.sin(2.0 * math.pi * 120.0 * t)
    env = 0.75 + 0.15 * math.sin(2.0 * math.pi * 120.0 * t)
    v = [a * env + ripple * 0.4, b * env + ripple * 0.35, c * env + ripple * 0.3, d * env + ripple * 0.25]
    return tuple(clamp(x) for x in v)  # type: ignore[return-value]


def wave_pwm_abnormal(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    a, b, c, d = base_wave(i, t, rng, sample_rate, sample_count)
    pwm = 0.6 * math.sin(2.0 * math.pi * 8000.0 * t) + 0.25 * math.sin(2.0 * math.pi * 16000.0 * t)
    jitter = 0.15 * math.sin(2.0 * math.pi * 12.0 * t)
    v = [a + pwm + rng.noise(0.05), b + pwm * 0.8 + jitter, c + pwm * 0.6 + jitter, d + pwm * 0.7 + rng.noise(0.05)]
    return tuple(clamp(x) for x in v)  # type: ignore[return-value]


def wave_igbt_fault(i: int, t: float, rng: XorShift32, sample_rate: int, sample_count: int) -> Tuple[float, float, float, float]:
    a, b, c, d = base_wave(i, t, rng, sample_rate, sample_count)
    # Periodic dropout/clamp windows every 20ms (~50Hz)
    win = i % int(sample_rate / 50)  # 4800 @ 240k
    v = [a, b, c, d]
    if win < 240:
        v = [x * 0.05 for x in v]  # almost zero
    elif win < 480:
        v = [0.0, 0.0, 0.0, 0.0]
    elif win < 720:
        v = [clamp(x, -1.0, 1.0) for x in v]
    # Add hard clipping to mimic protection
    return tuple(clamp(x) for x in v)  # type: ignore[return-value]


def write_bin(path: str, sample_rate: int, sample_count: int, func: Callable[[int, float, XorShift32], Tuple[float, float, float, float]]) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)

    data_bytes = sample_count * CHANNELS * 2
    if DATA_OFFSET + data_bytes != PARTITION_BYTES:
        raise ValueError("size mismatch: header+data != 4MB partition")

    rng = XorShift32(seed=0xA5A5A5A5)
    checksum = 2166136261

    with open(path, "wb") as f:
        # Placeholder header; we'll rewrite after data is generated.
        f.write(b"\x00" * DATA_OFFSET)

        chunk_samples = 2048
        for base in range(0, sample_count, chunk_samples):
            n = min(chunk_samples, sample_count - base)
            out = bytearray()
            for j in range(n):
                idx = base + j
                t = idx / float(sample_rate)
                va, vb, vc, vd = func(idx, t, rng)
                out.extend(struct.pack(
                    "<4H",
                    voltage_to_code(va),
                    voltage_to_code(vb),
                    voltage_to_code(vc),
                    voltage_to_code(vd),
                ))
            checksum = checksum_update(checksum, out)
            f.write(out)

        # Write final header
        header = struct.pack(
            "<8I",
            MAGIC,
            VERSION,
            sample_rate,
            sample_count,
            CHANNELS,
            DATA_OFFSET,
            data_bytes,
            checksum,
        )
        f.seek(0)
        f.write(header + b"\x00" * (DATA_OFFSET - len(header)))

    size = os.path.getsize(path)
    if size != PARTITION_BYTES:
        raise RuntimeError(f"unexpected output size: {size} bytes")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate full 7-partition DAC8568 fault suite (7 x 4MB).")
    parser.add_argument("--out-dir", default="sd_card_payload/copy_to_sd/wave", help="Output directory for wave/*.bin")
    parser.add_argument("--sample-rate", type=int, default=240000)
    args = parser.parse_args()

    sample_rate = int(args.sample_rate)
    if sample_rate <= 0:
        raise ValueError("sample-rate must be positive")

    sample_count = int(FULL_SAMPLE_COUNT)

    out_dir = args.out_dir
    suite = [
        WaveSpec("normal", "normal.bin", lambda i, t, r: wave_normal(i, t, r, sample_rate, sample_count)),
        WaveSpec("ac_coupling", "ac_coupling.bin", lambda i, t, r: wave_ac_coupling(i, t, r, sample_rate, sample_count)),
        WaveSpec("bus_ground", "bus_ground.bin", lambda i, t, r: wave_bus_ground(i, t, r, sample_rate, sample_count)),
        WaveSpec("insulation", "insulation.bin", lambda i, t, r: wave_insulation(i, t, r, sample_rate, sample_count)),
        WaveSpec("cap_aging", "cap_aging.bin", lambda i, t, r: wave_cap_aging(i, t, r, sample_rate, sample_count)),
        WaveSpec("pwm_abnormal", "pwm_abnormal.bin", lambda i, t, r: wave_pwm_abnormal(i, t, r, sample_rate, sample_count)),
        WaveSpec("igbt_fault", "igbt_fault.bin", lambda i, t, r: wave_igbt_fault(i, t, r, sample_rate, sample_count)),
    ]

    for spec in suite:
        out_path = os.path.join(out_dir, spec.filename)
        print(f"[gen] {spec.name} -> {out_path}")
        write_bin(out_path, sample_rate, sample_count, spec.func)

    print("Done.")
    print(f"SampleRate: {sample_rate}")
    print(f"SampleCount: {sample_count}")
    print(f"EachFileBytes: {PARTITION_BYTES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
