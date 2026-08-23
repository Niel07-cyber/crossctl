#include "ctelegram/crc16.h"
#include "ctelegram/telegram.h"
#include "crossctl/server.h"

#include <cstdlib>

namespace crossctl { int selftest_statemachine(); }

#include <cstdio>
#include <cstring>
#include <string>

static const char *kVersion = "0.1.0";

static int failures = 0;

static void check(const char *what, bool ok)
{
    std::printf("  %-28s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) failures++;
}

static void hexdump(const uint8_t *p, size_t n)
{
    std::printf("  wire: ");
    for (size_t i = 0; i < n; i++) std::printf("%02X ", p[i]);
    std::printf("(%zu bytes)\n", n);
}

static int selftest()
{
    std::printf("crc16\n");
    const char *chk = "123456789";
    check("check vector 0x29B1",
          crc16_ccitt(reinterpret_cast<const uint8_t *>(chk), 9) == 0x29B1);
    check("empty input 0xFFFF",
          crc16_ccitt(reinterpret_cast<const uint8_t *>(""), 0) == 0xFFFF);

    std::printf("telegram\n");
    tg_frame_t tx;
    std::memset(&tx, 0, sizeof(tx));
    tx.type = 0x10;
    tx.seq  = 42;
    const char *body = "TRAIN_DETECTED";
    tx.len = static_cast<uint16_t>(std::strlen(body));
    std::memcpy(tx.payload, body, tx.len);

    uint8_t wire[TG_MAX_FRAME];
    size_t wire_len = 0;
    check("encode ok", tg_encode(&tx, wire, sizeof(wire), &wire_len) == TG_OK);
    check("encoded size", wire_len == TG_HEADER_LEN + tx.len + TG_CRC_LEN);
    hexdump(wire, wire_len);

    tg_frame_t rx;
    size_t consumed = 0;
    tg_status_t st = tg_decode(wire, wire_len, &rx, &consumed);
    std::string got(reinterpret_cast<char *>(rx.payload), rx.len);
    check("decode ok", st == TG_OK);
    check("round trip payload", got == body);
    check("round trip seq", rx.seq == 42);
    check("consumed all bytes", consumed == wire_len);

    std::printf("telegram: rejection cases\n");

    uint8_t bad[TG_MAX_FRAME];
    std::memcpy(bad, wire, wire_len);
    bad[TG_HEADER_LEN] ^= 0x01;
    check("corrupt payload -> crc",
          tg_decode(bad, wire_len, &rx, &consumed) == TG_ERR_BAD_CRC);

    std::memcpy(bad, wire, wire_len);
    bad[0] = 0x00;
    check("bad magic",
          tg_decode(bad, wire_len, &rx, &consumed) == TG_ERR_BAD_MAGIC);

    std::memcpy(bad, wire, wire_len);
    bad[2] = 0x99;
    check("bad version",
          tg_decode(bad, wire_len, &rx, &consumed) == TG_ERR_BAD_VERSION);

    check("truncated header",
          tg_decode(wire, 4, &rx, &consumed) == TG_ERR_TOO_SHORT);
    check("truncated body",
          tg_decode(wire, wire_len - 3, &rx, &consumed) == TG_ERR_TOO_SHORT);

    std::memcpy(bad, wire, wire_len);
    bad[6] = 0xFF; bad[7] = 0xFF;
    check("oversize length field",
          tg_decode(bad, wire_len, &rx, &consumed) == TG_ERR_BAD_LENGTH);

    size_t small_len = 0;
    check("encode into small buffer",
          tg_encode(&tx, wire, 4, &small_len) == TG_ERR_OVERFLOW);

    failures += crossctl::selftest_statemachine();

    std::printf("\n%s (%d failure(s))\n",
                failures ? "SELFTEST FAILED" : "SELFTEST OK", failures);
    return failures;
}

int main(int argc, char **argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("crossctl %s\n", kVersion);
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "--selftest") == 0) {
        return selftest() == 0 ? 0 : 1;
    }
    if (argc > 1 && std::strcmp(argv[1], "--serve") == 0) {
        crossctl::ServerConfig cfg;
        for (int i = 2; i + 1 < argc; i += 2) {
            if (std::strcmp(argv[i], "--port") == 0) {
                cfg.port = static_cast<uint16_t>(std::atoi(argv[i + 1]));
            } else if (std::strcmp(argv[i], "--timeout") == 0) {
                cfg.move_timeout = static_cast<uint32_t>(std::atoi(argv[i + 1]));
            }
        }
        return crossctl::run_server(cfg);
    }
    std::printf("crossctl %s — try --version, --selftest or --serve\n", kVersion);
    return 0;
}
