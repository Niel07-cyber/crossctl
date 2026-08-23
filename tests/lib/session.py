#!/usr/bin/env python3
"""Send frames to crossctl and print decoded replies.

usage: session.py PORT [--burst|--split] EVENT [EVENT...]

Event syntax:
  EVENT           normal frame
  !EVENT          corrupt this frame's CRC
  @0xNN:EVENT     override the telegram type byte

Delivery modes (they exercise the server's reassembly, not its logic):
  default         one write per frame
  --burst         every frame in a single write
  --split         all bytes in two writes, split mid-frame
"""
import socket, sys, time

sys.path.insert(0, __file__.rsplit("/", 1)[0])
from mkframe import build  # noqa: E402

NAMES = {0x10: "EVENT", 0x20: "STATUS", 0x30: "ERROR"}
MAGIC = 0xC51C


def decode_all(data: bytes):
    i, out = 0, []
    while i + 8 <= len(data):
        if int.from_bytes(data[i:i+2], "big") != MAGIC:
            i += 1
            continue
        ln = int.from_bytes(data[i+6:i+8], "big")
        end = i + 8 + ln + 2
        if end > len(data):
            break
        out.append(f"{NAMES.get(data[i+3], hex(data[i+3]))} "
                   f"{int.from_bytes(data[i+4:i+6], 'big')} "
                   f"{data[i+8:i+8+ln].decode('utf-8', 'replace')}")
        i = end
    return out


def frame_for(spec: str, seq: int) -> bytes:
    corrupt = spec.startswith("!")
    if corrupt:
        spec = spec[1:]
    type_ = 0x10
    if spec.startswith("@"):
        head, spec = spec[1:].split(":", 1)
        type_ = int(head, 0)
    return build(spec.encode(), seq=seq, type_=type_, corrupt=corrupt)


def main():
    args = sys.argv[1:]
    port = int(args[0])
    mode = "each"
    rest = []
    for a in args[1:]:
        if a == "--burst":
            mode = "burst"
        elif a == "--split":
            mode = "split"
        else:
            rest.append(a)

    frames = [frame_for(spec, n) for n, spec in enumerate(rest, start=1)]

    s = socket.create_connection(("127.0.0.1", port), timeout=3)

    if mode == "burst":
        s.sendall(b"".join(frames))
    elif mode == "split":
        blob = b"".join(frames)
        cut = max(1, len(blob) // 2)
        s.sendall(blob[:cut])
        time.sleep(0.15)          # server must hold the partial frame
        s.sendall(blob[cut:])
    else:
        for f in frames:
            s.sendall(f)
            time.sleep(0.02)

    s.shutdown(socket.SHUT_WR)
    s.settimeout(1.0)
    buf = b""
    deadline = time.time() + 2.0
    while time.time() < deadline:
        try:
            chunk = s.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        buf += chunk
    s.close()
    for line in decode_all(buf):
        print(line)


if __name__ == "__main__":
    main()
