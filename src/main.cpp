#include "ctelegram/crc16.h"

#include <cstdio>
#include <cstring>

static const char *kVersion = "0.1.0";

static int selftest()
{
    int failures = 0;

    const char *chk = "123456789";
    uint16_t crc = crc16_ccitt(reinterpret_cast<const uint8_t *>(chk), 9);
    std::printf("crc16(\"123456789\") = 0x%04X (expect 0x29B1) %s\n",
                crc, crc == 0x29B1 ? "PASS" : "FAIL");
    if (crc != 0x29B1) failures++;

    uint16_t empty = crc16_ccitt(reinterpret_cast<const uint8_t *>(""), 0);
    std::printf("crc16(\"\")          = 0x%04X (expect 0xFFFF) %s\n",
                empty, empty == 0xFFFF ? "PASS" : "FAIL");
    if (empty != 0xFFFF) failures++;

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
    std::printf("crossctl %s — try --version or --selftest\n", kVersion);
    return 0;
}
