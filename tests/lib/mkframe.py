#!/usr/bin/env python3
"""Emit a crossctl telegram on stdout as raw bytes.

usage: mkframe.py EVENT [--seq N] [--type N] [--corrupt]
"""
import sys

MAGIC, VERSION = 0xC51C, 0x01

def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def build(payload: bytes, seq: int = 1, type_: int = 0x10, corrupt: bool = False) -> bytes:
    head = (MAGIC.to_bytes(2, "big") + bytes([VERSION, type_])
            + seq.to_bytes(2, "big") + len(payload).to_bytes(2, "big"))
    body = head + payload
    crc = crc16(body)
    if corrupt:
        crc ^= 0x0001
    return body + crc.to_bytes(2, "big")

if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    event = args[0].encode()
    seq, type_, corrupt = 1, 0x10, False
    i = 1
    while i < len(args):
        if args[i] == "--seq":     seq = int(args[i + 1]); i += 2
        elif args[i] == "--type":  type_ = int(args[i + 1], 0); i += 2
        elif args[i] == "--corrupt": corrupt = True; i += 1
        else: i += 1
    try:
        sys.stdout.buffer.write(build(event, seq, type_, corrupt))
        sys.stdout.buffer.flush()
    except BrokenPipeError:
        sys.exit(1)
