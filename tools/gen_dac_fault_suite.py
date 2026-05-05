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


def cos_cycles(ctx: "WaveContext", i: int, cycles: int) -> float:
    if cycles == 0:
        return 1.0
    return math.cos(ctx.two_pi_over_n * float(cycles) * float(i))


def exp_decay(samples: int, tau_samples: float) -> float:
    return math.exp(-float(max(0, samples)) / max(1.0, tau_samples))


def damped_ring(samples: int, sample_rate: int, freq_hz: float, tau_s: float, amp: float) -> float:
    if samples < 0:
        return 0.0
    t = float(samples) / float(sample_rate)
    return amp * math.exp(-t / max(1.0e-6, tau_s)) * math.sin(2.0 * math.pi * freq_hz * t)


def square_from_sine(v: float) -> float:
    return 1.0 if v >= 0.0 else -1.0


@dataclass(frozen=True)
class WaveContext:
    sample_rate: int
    sample_count: int
    two_pi_over_n: float

    cyc_10: int
    cyc_50: int
    cyc_100: int
    cyc_120: int
    cyc_300: int
    cyc_600: int
    cyc_2k: int
    cyc_4k: int
    cyc_6k: int
    cyc_8k: int
    cyc_9k: int
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
        cyc_300=cycles_for_hz(300.0, sample_rate, sample_count),
        cyc_600=cycles_for_hz(600.0, sample_rate, sample_count),
        cyc_2k=cycles_for_hz(2000.0, sample_rate, sample_count),
        cyc_4k=cycles_for_hz(4000.0, sample_rate, sample_count),
        cyc_6k=cycles_for_hz(6000.0, sample_rate, sample_count),
        cyc_8k=cycles_for_hz(8000.0, sample_rate, sample_count),
        cyc_9k=cycles_for_hz(9000.0, sample_rate, sample_count),
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
    s300 = ctx.sin_cycles(i, ctx.cyc_300)
    s2k = ctx.sin_cycles(i, ctx.cyc_2k)
    s8k = ctx.sin_cycles(i, ctx.cyc_8k)

    # AC coupled interference mainly appears as common-mode 50Hz motion,
    # with a smaller differential ripple and zero-crossing switching noise.
    zero_cross = max(0.0, 1.0 - abs(s50) * 10.0)
    burst = zero_cross * (0.10 * s2k + 0.04 * s8k)
    cm = 0.55 * s50 + 0.10 * s300 + burst
    diff = 0.05 * s100 + 0.07 * s50

    a = 3.00 + diff + cm
    b = -3.00 - diff + cm
    c = 1.45 + 0.08 * s100 + 0.32 * abs(s50) + 0.10 * s300 + rng.noise(0.015)
    d = 0.05 + 0.18 * abs(s50) + 0.04 * zero_cross + rng.noise(0.012)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_bus_ground(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    a = 3.00 + 0.03 * s100
    b = -3.00 - 0.03 * s100
    c = 1.50 + 0.06 * s100
    d = 0.05 + rng.noise(0.005)

    win = i % ctx.sag_period_samples
    if win < ctx.sag_len_samples * 3:
        decay = exp_decay(win, ctx.sag_tau_samples)
        ring = damped_ring(win, ctx.sample_rate, 3600.0, 0.004, 0.18)
        short = 1.0 if win < ctx.surge_len_samples else exp_decay(win - ctx.surge_len_samples, ctx.sag_tau_samples * 1.8)

        # Positive bus is pulled hard toward ground; negative rail shifts toward
        # ground less aggressively. Current surges first, then protection limits.
        a = 0.18 + 0.50 * (1.0 - decay) + ring
        b = -3.00 + 1.55 * short - 0.10 * ring
        c = 4.40 * decay if win < ctx.surge_len_samples else 0.35 + 0.65 * short
        d = 0.30 + 1.45 * decay + 0.10 * abs(ring)
    elif win < ctx.sag_len_samples * 7:
        rec = 1.0 - exp_decay(win - ctx.sag_len_samples * 3, ctx.sag_tau_samples * 8.0)
        a = 0.75 + 2.25 * rec + 0.02 * s100
        b = -1.65 - 1.35 * rec - 0.02 * s100
        c = 0.55 + 0.55 * rec
        d = 0.16 + 0.25 * (1.0 - rec)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_insulation(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    slow = 0.5 * (1.0 + ctx.sin_cycles(i, ctx.drift_cyc_1))
    asym = 0.22 * ctx.sin_cycles(i, ctx.drift_cyc_2)
    leakage = 0.18 + 0.52 * slow
    partial = 0.0

    # Deterministic partial-discharge bursts. They are narrow, random-looking,
    # and ride mostly on leakage plus small bus dents.
    if (i % 4093) < 46:
        partial += 0.42 * exp_decay(i % 4093, 14.0)
    if (i % 7919) < 80:
        partial += 0.25 * exp_decay(i % 7919, 28.0)

    a = 3.00 + 0.04 * s100 + asym - 0.06 * partial + rng.noise(0.025)
    b = -3.00 - 0.04 * s100 + asym + 0.05 * partial + rng.noise(0.025)
    c = 1.35 + 0.09 * s100 + 0.16 * slow + rng.noise(0.04)
    d = leakage + partial + rng.noise(0.018)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_cap_aging(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    s120 = ctx.sin_cycles(i, ctx.cyc_120)
    c100 = cos_cycles(ctx, i, ctx.cyc_100)
    phase = (i % max(1, ctx.sample_rate // 100)) / float(max(1, ctx.sample_rate // 100))

    # Aged DC-link capacitors show high ripple, deeper valleys between charge
    # peaks, and ESR-related current ripple/spikes around recharge instants.
    recharge_spike = exp_decay(int(phase * 220.0), 18.0) if phase < 0.16 else 0.0
    valley = (1.0 - c100) * 0.5
    ripple = 1.05 * s100 + 0.30 * s120 - 0.42 * valley

    a = 2.62 + ripple + 0.13 * recharge_spike
    b = -2.62 - ripple - 0.13 * recharge_spike
    c = 1.35 + 0.58 * max(0.0, s100) + 0.32 * recharge_spike + rng.noise(0.025)
    d = 0.06 + 0.015 * abs(s120) + rng.noise(0.006)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_pwm_abnormal(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    s6k = ctx.sin_cycles(i, ctx.cyc_6k)
    s8k = ctx.sin_cycles(i, ctx.cyc_8k)
    s9k = ctx.sin_cycles(i, ctx.cyc_9k)
    s12k = ctx.sin_cycles(i, ctx.cyc_12k)
    s10 = ctx.sin_cycles(i, ctx.cyc_10)
    dropout = 1.0 if (i % 3072) < 210 else 0.0
    duty_jump = 1.0 if (i % 8192) < 900 else -0.35

    carrier = 0.22 * s8k + 0.12 * square_from_sine(s9k + 0.25 * s10) + 0.06 * s12k
    beat = 0.18 * s6k * s10
    missing = -0.55 * dropout * exp_decay(i % 3072, 120.0)

    a = 3.00 + 0.04 * s100 + 0.22 * carrier + 0.06 * duty_jump + missing * 0.18
    b = -3.00 - 0.04 * s100 + 0.22 * carrier - 0.06 * duty_jump - missing * 0.12
    c = 1.42 + 0.08 * s100 + 0.72 * carrier + beat + 0.45 * dropout + rng.noise(0.035)
    d = 0.06 + 0.06 * abs(carrier) + 0.03 * dropout + rng.noise(0.012)

    return (
        clamp_bipolar(a),
        clamp_bipolar(b),
        clamp_unipolar(c),
        clamp_unipolar(d),
    )


def wave_igbt_fault(i: int, rng: XorShift32, ctx: WaveContext) -> Tuple[float, float, float, float]:
    s100 = ctx.sin_cycles(i, ctx.cyc_100)
    a = 3.00 + 0.03 * s100
    b = -3.00 - 0.03 * s100
    c = 1.50 + 0.06 * s100
    d = 0.05 + rng.noise(0.005)

    win = i % ctx.igbt_period_samples
    if win < ctx.igbt_w1_samples:
        decay = exp_decay(win, ctx.igbt_tau_samples)
        ring = damped_ring(win, ctx.sample_rate, 5200.0, 0.0022, 0.28)
        c = 4.75 * decay + 1.10 * (1.0 - decay)
        a = a - 1.45 * decay + ring
        b = b + 1.25 * decay - 0.7 * ring
        d = d + 0.78 * decay + 0.10 * abs(ring)
    elif win < (ctx.igbt_w1_samples + ctx.igbt_w2_samples * 3):
        t = win - ctx.igbt_w1_samples
        rec = 1.0 - exp_decay(t, ctx.igbt_tau_samples * 2.2)
        clamp_window = 1.0 - rec
        c = 0.18 + 0.55 * rec
        a = 1.45 + 1.55 * rec
        b = -1.35 - 1.65 * rec
        d = d + 0.22 * clamp_window
    elif (win > ctx.igbt_period_samples - ctx.igbt_w2_samples) and (win < ctx.igbt_period_samples):
        pre = ctx.igbt_period_samples - win
        c = c + 0.35 * exp_decay(pre, ctx.igbt_tau_samples)

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
