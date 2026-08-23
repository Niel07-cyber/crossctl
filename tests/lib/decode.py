#!/usr/bin/env python3
"""Read raw telegram frames from stdin, print 'TYPE SEQ PAYLOAD' per line."""
import sys

MAGIC = 0xC51C
NAMES = {0x10: "EVENT", 0x20: "STATUS", 0x30: "ERROR"}

data = sys.stdin.buffer.read()
i = 0
while i + 8 <= len(data):
    if int.from_bytes(data[i:i+2], "big") != MAGIC:
        i += 1
        continue
    ln = int.from_bytes(data[i+6:i+8], "big")
    end = i + 8 + ln + 2
    if end > len(data):
        break
    typ = data[i+3]
    seq = int.from_bytes(data[i+4:i+6], "big")
    payload = data[i+8:i+8+ln].decode("utf-8", "replace")
    print(f"{NAMES.get(typ, hex(typ))} {seq} {payload}")
    i = end
