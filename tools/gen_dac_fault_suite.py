#!/usr/bin/env python3
"""
Generate 7 x 4MB DAC8568 waveform binaries for SD -> W25Q256 full-sync.

Channel semantics (external domain, +/-5V):
  A: DC bus positive voltage (bipolar, normal positive)
  B: DC bus negative voltage (bipolar, normal negative)
  C: Load current (unipolar, >=0)
  D: Leakage current (unipolar, >=0)

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
from typing import Callable, Tuple


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


def clamp_bipolar(v: float) -> float:
    return clamp(v, VOUT_MIN, VOUT_MAX)


def clamp_unipolar(v: float) -> float:
    return clamp(v, 0.0, VOUT_MAX)


def cycles_for_hz(hz_target: float, sample_rate: int, sample_count: int) -> int:
    if hz_target <= 0.0:
        return 0
    cycles = int(round(hz_target * sample_count / float(sample_rate)))
    return max(1, cycles)


@dataclass(frozen=True)
class WaveContext:
    sample_rate: int
    sample_count: int
    two_pi_over_n: float

    cyc_10: int
    cyc_50: int
    cyc_100: int
    cyc_120: int
    cyc_8k: int
    cyc_12k: int

    drift_cyc_1: int
    drift_cyc_2: int

    sag_period_samples: int
    sag_len_samples: int
    surge_len_samples: int
    sag_tau_samples: float

    igbt_period_samples: int
    igbt_w1_samples: int
    igbt_w2_samples: int
    igbt_tau_samples: float

    def sin_cycles(self, i: int, cycles: int) -> float:
        if cycles == 0:
            return 0.0
        return math.sin(self.two_pi_over_n * float(cycles) * float(i))


@dataclass(frozen=True)
class WaveSpec:
    name: str
    filename: str
    func: Callable[[int, XorShift32], Tuple[float, float, float, float]]


def make_context(sample_rate: int, sample_count: int) -> WaveContext:
    two_pi_over_n = 2.0 * math.pi / float(sample_count)

    sag_period_samples = max(1, int(round(sample_rate / 10.0)))  # ~100ms @ 10Hz
    sag_len_samples = max(1, int(round(sample_rate * 0.005)))  # 5ms sag window
    surge_len_samples = max(1, int(round(sample_rate * 0.001)))  # 1ms surge
    sag_tau_samples = max(1.0, float(sample_rate) * 0.002)  # 2ms decay

    igbt_period_samples = max(1, int(round(sample_rate / 50.0)))  # 20ms @ 50Hz
    igbt_w1_samples = max(1, int(round(sample_rate * 0.001)))  # ~1ms
    igbt_w2_samples = max(1, int(round(sample_rate * 0.001)))  # ~1ms
    igbt_tau_samples = max(1.0, float(sample_rate) * 0.0015)  # 1.5ms decay

    return WaveContext(
        sample_rate=sample_rate,
        sample_count=sample_count,
        two_pi_over_n=two_pi_over_n,
        cyc_10=cycles_for_hz(10.0, sample_rate, sample_count),
        cyc_50=cycles_for_hz(50.0, sample_rate, sample_count),
        cyc_100=cycles_for_hz(100.0, sample_rate, sample_count),
        cyc_120=cycles_for_hz(120.0, sample_rate, sample_count),
        cyc_8k=cycles_for_hz(8000.0, sample_rate, sample_count),
        cyc_12k=cycles_for_hz(12000.0, sample_rate, sample_count),
        drift_cyc_1=1,
        drift_cyc_2=2,
        sag_period_samples=sag_period_samples,
        sag_len_samples=sag_len_samples,
        surge_len_samples=surge_len_samples,
        sag_tau_samples=sag_tau_samples,
        igbt_period_samples=igbt_period_samples,
        igbt_w1_samples=igbt_w1_samples,
        igbt_w2_samples=igbt_w2_samples,
        igbt_tau_samples=igbt_tau_samples,
    )


def wave_baseline(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    # Normal DC bus with small differential 100Hz ripple; currents are unipolar.
    s100 = ctx.sin_cycles(i, ctx.cyc_100)

    a = 3.00 + 0.03 * s100
    b = -3.00 - 0.03 * s100
    c = 1.50 + 0.06 * s100
    d = 0.05 + rng.noise(0.005)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_normal(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    return wave_baseline(i, rng, ctx)


def wave_ac_coupling(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s50 = ctx.sin_cycles(i, ctx.cyc_50)
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    s8k = ctx.sin_cycles(i, ctx.cyc_8k)
    s12k = ctx.sin_cycles(i, ctx.cyc_12k)

    # Independent fault waveform: base DC bus around +/-3V with distinct common-mode
    # AC coupling interference plus modest HF ripple (<= 12.8kHz).
    cm = 0.35 * s50 + 0.18 * s100
    hf = 0.05 * s8k + 0.02 * s12k

    a = 3.00 + 0.03 * s100 + cm + hf
    b = -3.00 - 0.03 * s100 + cm + hf
    c = 1.50 + 0.06 * s100 + 0.35 * s50 + 0.18 * s100
    d = 0.05 + 0.05 * abs(s50) + rng.noise(0.01)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_bus_ground(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    # Independent fault waveform with periodic bus sag window.
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    a = 3.00 + 0.03 * s100
    b = -3.00 - 0.03 * s100
    c = 1.50 + 0.06 * s100
    d = 0.05 + rng.noise(0.005)

    win = i % ctx.sag_period_samples
    if win < ctx.sag_len_samples:
        a = 0.5
        b = b + 1.3  # drift towards 0V

        if win < ctx.surge_len_samples:
            c = 3.0
        else:
            c = 0.2

        d = 0.20 + 1.00 * math.exp(-float(win) / ctx.sag_tau_samples)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_insulation(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    # Independent fault waveform: slow drift + increased noise + leakage rise.
    s100 = ctx.sin_cycles(i, ctx.cyc_100)

    drift = 0.20 * ctx.sin_cycles(i, ctx.drift_cyc_2)
    slow = 0.08 * ctx.sin_cycles(i, ctx.drift_cyc_1)

    a = 3.00 + 0.03 * s100 + drift + rng.noise(0.02)
    b = -3.00 - 0.03 * s100 + drift + rng.noise(0.02)
    c = 1.50 + 0.06 * s100 + slow + rng.noise(0.05)

    # Slow leakage rise (0.05V -> ~0.5V) with added noise; keep unipolar.
    env = 0.5 * (1.0 + ctx.sin_cycles(i, ctx.drift_cyc_1))
    d = 0.05 + 0.45 * env + rng.noise(0.02)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_cap_aging(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    s120 = ctx.sin_cycles(i, ctx.cyc_120)

    # Independent fault waveform: increased 100/120Hz ripple and slightly reduced average bus.
    a = 2.40 + 1.80 * s100 + 0.30 * s120
    b = -2.40 - 1.80 * s100 - 0.30 * s120
    c = 1.50 + 0.80 * s100 + 0.15 * s120
    d = 0.05 + rng.noise(0.005)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_pwm_abnormal(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    # Independent fault waveform: PWM ripple (<= 12.8kHz) + modulation on current.
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    s8k = ctx.sin_cycles(i, ctx.cyc_8k)
    s12k = ctx.sin_cycles(i, ctx.cyc_12k)
    s10 = ctx.sin_cycles(i, ctx.cyc_10)

    a = 3.00 + 0.03 * s100 + 0.10 * s8k + 0.05 * s12k
    b = -3.00 - 0.03 * s100 + 0.10 * s8k + 0.05 * s12k

    mod = 1.0 + 0.2 * s10
    c = 1.50 + 0.06 * s100 + mod * (0.50 * s8k + 0.20 * s12k) + rng.noise(0.03)

    d = 0.05 + 0.03 * s8k + rng.noise(0.01)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_igbt_fault(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    # Independent fault waveform: periodic drop/clamp windows on current with bus sag and leakage spikes.
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    a = 3.00 + 0.03 * s100
    b = -3.00 - 0.03 * s100
    c = 1.50 + 0.06 * s100
    d = 0.05 + rng.noise(0.005)

    win = i % ctx.igbt_period_samples
    if win < ctx.igbt_w1_samples:
        c = 0.0
        a = a - 1.2
        b = b + 1.2
        d = d + 0.5 * math.exp(-float(win) / ctx.igbt_tau_samples)
    elif win < (ctx.igbt_w1_samples + ctx.igbt_w2_samples):
        c = 0.4
        a = a - 0.6
        b = b + 0.6
        d = d + 0.25 * math.exp(-float(win - ctx.igbt_w1_samples) / ctx.igbt_tau_samples)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def write_bin(path: str, sample_rate: int, sample_count: int, func: Callable[[int, XorShift32], Tuple[float, float, float, float]]) -> None:
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
                va, vb, vc, vd = func(idx, rng)
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
    parser.add_argument("--sample-rate", type=int, default=102400)
    args = parser.parse_args()

    sample_rate = int(args.sample_rate)
    if sample_rate <= 0:
        raise ValueError("sample-rate must be positive")

    sample_count = int(FULL_SAMPLE_COUNT)
    ctx = make_context(sample_rate, sample_count)

    out_dir = args.out_dir
    suite = [
        WaveSpec("normal", "normal.bin", lambda i, r: wave_normal(i, r, ctx)),
        WaveSpec("ac_coupling", "ac_coupling.bin", lambda i, r: wave_ac_coupling(i, r, ctx)),
        WaveSpec("bus_ground", "bus_ground.bin", lambda i, r: wave_bus_ground(i, r, ctx)),
        WaveSpec("insulation", "insulation.bin", lambda i, r: wave_insulation(i, r, ctx)),
        WaveSpec("cap_aging", "cap_aging.bin", lambda i, r: wave_cap_aging(i, r, ctx)),
        WaveSpec("pwm_abnormal", "pwm_abnormal.bin", lambda i, r: wave_pwm_abnormal(i, r, ctx)),
        WaveSpec("igbt_fault", "igbt_fault.bin", lambda i, r: wave_igbt_fault(i, r, ctx)),
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
