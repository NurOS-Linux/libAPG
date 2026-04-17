// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <string.h>
#include <stdio.h>

#include "../../include/apg/md5.h"

#define F(b,c,d) ((b & c) | (~b & d))
#define G(b,c,d) ((b & d) | (c & ~d))
#define H(b,c,d) (b ^ c ^ d)
#define I(b,c,d) (c ^ (b | ~d))
#define ROTL(x,n) (((x) << (n)) | ((x) >> (32 - (n))))

static const uint32_t T[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint8_t S[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static void
md5_transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t M[16];

    for (int i = 0; i < 16; i++) {
        M[i] = (uint32_t)block[i*4]
             | ((uint32_t)block[i*4+1] << 8)
             | ((uint32_t)block[i*4+2] << 16)
             | ((uint32_t)block[i*4+3] << 24);
    }

    for (int i = 0; i < 64; i++) {
        uint32_t f, g;
        if      (i < 16) { f = F(b,c,d); g = i; }
        else if (i < 32) { f = G(b,c,d); g = (5*i + 1) % 16; }
        else if (i < 48) { f = H(b,c,d); g = (3*i + 5) % 16; }
        else             { f = I(b,c,d); g = (7*i)     % 16; }

        uint32_t tmp = d;
        d = c;
        c = b;
        b = b + ROTL(a + f + M[g] + T[i], S[i]);
        a = tmp;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

void
md5_init(md5_ctx *ctx)
{
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->count[0] = ctx->count[1] = 0;
}

void
md5_update(md5_ctx *ctx, const uint8_t *data, size_t len)
{
    uint32_t idx = (ctx->count[0] >> 3) & 0x3f;

    ctx->count[0] += (uint32_t)(len << 3);
    if (ctx->count[0] < (uint32_t)(len << 3))
        ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);

    uint32_t part = 64 - idx;
    size_t i;

    if (len >= part) {
        memcpy(&ctx->buf[idx], data, part);
        md5_transform(ctx->state, ctx->buf);
        for (i = part; i + 63 < len; i += 64)
            md5_transform(ctx->state, &data[i]);
        idx = 0;
    } else {
        i = 0;
    }

    memcpy(&ctx->buf[idx], &data[i], len - i);
}

void
md5_final(uint8_t digest[16], md5_ctx *ctx)
{
    static const uint8_t pad[64] = { 0x80 };
    uint8_t bits[8];

    for (int i = 0; i < 4; i++) {
        bits[i]   = (ctx->count[0] >> (i * 8)) & 0xff;
        bits[i+4] = (ctx->count[1] >> (i * 8)) & 0xff;
    }

    uint32_t idx     = (ctx->count[0] >> 3) & 0x3f;
    uint32_t pad_len = (idx < 56) ? (56 - idx) : (120 - idx);
    md5_update(ctx, pad, pad_len);
    md5_update(ctx, bits, 8);

    for (int i = 0; i < 4; i++) {
        digest[i*4]   =  ctx->state[i]        & 0xff;
        digest[i*4+1] = (ctx->state[i] >>  8) & 0xff;
        digest[i*4+2] = (ctx->state[i] >> 16) & 0xff;
        digest[i*4+3] = (ctx->state[i] >> 24) & 0xff;
    }
}

bool
compute_md5(const char *path, uint8_t digest[16])
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    md5_ctx ctx;
    md5_init(&ctx);

    uint8_t buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        md5_update(&ctx, buf, n);

    fclose(f);
    md5_final(digest, &ctx);
    return true;
}
