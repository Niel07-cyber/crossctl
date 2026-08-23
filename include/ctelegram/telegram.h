#ifndef CTELEGRAM_TELEGRAM_H
#define CTELEGRAM_TELEGRAM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TG_MAGIC        0xC51Cu
#define TG_VERSION      0x01u
#define TG_HEADER_LEN   8u
#define TG_CRC_LEN      2u
#define TG_MAX_PAYLOAD  256u
#define TG_MAX_FRAME    (TG_HEADER_LEN + TG_MAX_PAYLOAD + TG_CRC_LEN)

typedef enum {
    TG_OK = 0,
    TG_ERR_TOO_SHORT    = 1,
    TG_ERR_BAD_MAGIC    = 2,
    TG_ERR_BAD_VERSION  = 3,
    TG_ERR_BAD_LENGTH   = 4,
    TG_ERR_BAD_CRC      = 5,
    TG_ERR_OVERFLOW     = 6
} tg_status_t;

typedef struct {
    uint8_t  type;
    uint16_t seq;
    uint16_t len;
    uint8_t  payload[TG_MAX_PAYLOAD];
} tg_frame_t;

tg_status_t tg_encode(const tg_frame_t *f, uint8_t *out,
                      size_t out_cap, size_t *out_len);

tg_status_t tg_decode(const uint8_t *in, size_t in_len,
                      tg_frame_t *f, size_t *consumed);

const char *tg_strerror(tg_status_t s);

#ifdef __cplusplus
}
#endif

#endif
