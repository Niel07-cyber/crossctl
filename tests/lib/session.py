#!/usr/bin/env python3
"""Send frames to crossctl and print decoded replies.

usage: session.py PORT EVENT[...]   ('!EVENT' corrupts that frame's CRC)
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

def main():
    port, events = int(sys.argv[1]), sys.argv[2:]
    s = socket.create_connection(("127.0.0.1", port), timeout=3)
    for n, ev in enumerate(events, start=1):
        corrupt = ev.startswith("!")
        s.sendall(build((ev[1:] if corrupt else ev).encode(), seq=n, corrupt=corrupt))
        time.sleep(0.02)

    s.shutdown(socket.SHUT_WR)          # signal EOF, keep reading replies
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
