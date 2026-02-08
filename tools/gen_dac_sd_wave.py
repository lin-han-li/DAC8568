#!/usr/bin/env python3
import argparse
import math
import os
import struct

MAGIC = 0x44384357  # "D8CW" (legacy format used by commit 504cb0...)
VERSION = 1
CHANNELS = 4
DATA_OFFSET = 64
VREF_MV = 2500.0
VOUT_MAX = 5.0
VOUT_MIN = -5.0


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


def checksum_fnv1a(data: bytes) -> int:
    value = 2166136261
    for byte in data:
        value = (value * 16777619) & 0xFFFFFFFF
        value ^= byte
    return value


def triangle(phase: float) -> float:
    x = phase % 1.0
    if x < 0.5:
        return 4.0 * x - 1.0
    return 3.0 - 4.0 * x


def saw(phase: float) -> float:
    x = phase % 1.0
    return 2.0 * x - 1.0


def square(phase: float) -> float:
    return 1.0 if (phase % 1.0) < 0.5 else -1.0


def build_wave(sample_rate: int, sample_count: int, fa: float, fb: float, fc: float, fd: float) -> bytes:
    words = []
    for index in range(sample_count):
        t = index / float(sample_rate)
        va = VOUT_MAX * math.sin(2.0 * math.pi * fa * t)
        vb = VOUT_MAX * triangle(fb * t)
        vc = VOUT_MAX * saw(fc * t)
        vd = VOUT_MAX * square(fd * t)
        words.extend(
            [
                voltage_to_code(va),
                voltage_to_code(vb),
                voltage_to_code(vc),
                voltage_to_code(vd),
            ]
        )
    return struct.pack("<{}H".format(len(words)), *words)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate DAC8568 SD waveform binary for SD->W25Q sync.")
    parser.add_argument("--output", default="sd_card_payload/wave/dac8568_wave.bin")
    parser.add_argument("--sample-rate", type=int, default=240000)
    parser.add_argument("--samples", type=int, default=4096)
    parser.add_argument("--fa", type=float, default=12000.0)
    parser.add_argument("--fb", type=float, default=6000.0)
    parser.add_argument("--fc", type=float, default=3000.0)
    parser.add_argument("--fd", type=float, default=1000.0)
    args = parser.parse_args()

    if args.sample_rate <= 0 or args.samples <= 0:
        raise ValueError("sample-rate and samples must be positive")

    data = build_wave(args.sample_rate, args.samples, args.fa, args.fb, args.fc, args.fd)
    checksum = checksum_fnv1a(data)
    header = struct.pack(
        "<8I",
        MAGIC,
        VERSION,
        args.sample_rate,
        args.samples,
        CHANNELS,
        DATA_OFFSET,
        len(data),
        checksum,
    )
    payload = header + bytes(DATA_OFFSET - len(header)) + data

    out_dir = os.path.dirname(args.output)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(args.output, "wb") as file:
        file.write(payload)

    print("Generated:", args.output)
    print("SampleRate:", args.sample_rate)
    print("Samples:", args.samples)
    print("DataBytes:", len(data))
    print("Checksum: 0x{:08X}".format(checksum))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

