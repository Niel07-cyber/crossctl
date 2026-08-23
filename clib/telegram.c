#include "ctelegram/telegram.h"
#include "ctelegram/crc16.h"

#include <string.h>

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFFu);
}

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

tg_status_t tg_encode(const tg_frame_t *f, uint8_t *out,
                      size_t out_cap, size_t *out_len)
{
    if (f == NULL || out == NULL || out_len == NULL) {
        return TG_ERR_OVERFLOW;
    }
    if (f->len > TG_MAX_PAYLOAD) {
        return TG_ERR_BAD_LENGTH;
    }

    size_t total = TG_HEADER_LEN + (size_t)f->len + TG_CRC_LEN;
    if (out_cap < total) {
        return TG_ERR_OVERFLOW;
    }

    put_u16(&out[0], TG_MAGIC);
    out[2] = TG_VERSION;
    out[3] = f->type;
    put_u16(&out[4], f->seq);
    put_u16(&out[6], f->len);
    memcpy(&out[TG_HEADER_LEN], f->payload, f->len);

    uint16_t crc = crc16_ccitt(out, TG_HEADER_LEN + (size_t)f->len);
    put_u16(&out[TG_HEADER_LEN + f->len], crc);

    *out_len = total;
    return TG_OK;
}

tg_status_t tg_decode(const uint8_t *in, size_t in_len,
                      tg_frame_t *f, size_t *consumed)
{
    if (in == NULL || f == NULL || consumed == NULL) {
        return TG_ERR_OVERFLOW;
    }
    if (in_len < TG_HEADER_LEN) {
        return TG_ERR_TOO_SHORT;
    }
    if (get_u16(&in[0]) != TG_MAGIC) {
        return TG_ERR_BAD_MAGIC;
    }
    if (in[2] != TG_VERSION) {
        return TG_ERR_BAD_VERSION;
    }

    uint16_t len = get_u16(&in[6]);
    if (len > TG_MAX_PAYLOAD) {
        return TG_ERR_BAD_LENGTH;
    }

    size_t total = TG_HEADER_LEN + (size_t)len + TG_CRC_LEN;
    if (in_len < total) {
        return TG_ERR_TOO_SHORT;
    }

    uint16_t want = crc16_ccitt(in, TG_HEADER_LEN + (size_t)len);
    uint16_t got  = get_u16(&in[TG_HEADER_LEN + len]);
    if (want != got) {
        return TG_ERR_BAD_CRC;
    }

    f->type = in[3];
    f->seq  = get_u16(&in[4]);
    f->len  = len;
    memcpy(f->payload, &in[TG_HEADER_LEN], len);

    *consumed = total;
    return TG_OK;
}

const char *tg_strerror(tg_status_t s)
{
    switch (s) {
    case TG_OK:              return "ok";
    case TG_ERR_TOO_SHORT:   return "frame too short";
    case TG_ERR_BAD_MAGIC:   return "bad magic";
    case TG_ERR_BAD_VERSION: return "bad version";
    case TG_ERR_BAD_LENGTH:  return "bad length";
    case TG_ERR_BAD_CRC:     return "crc mismatch";
    case TG_ERR_OVERFLOW:    return "buffer overflow";
    default:                 return "unknown";
    }
}
