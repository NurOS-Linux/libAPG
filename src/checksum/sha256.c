// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 XCubicArnament <xcubicarnament@nuros.org>
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
//
// SHA-256 implementation (FIPS 180-4).
// Hardware acceleration via SHA-NI (x86_64), ARMv8 Crypto (aarch64),
// or RISC-V Zknh (riscv64) when available; falls back to portable C.

#include "../../include/apg/sha256.h"
#include <stdio.h>
#include <string.h>

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x,y,z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)     (ROTR(x,2)  ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x)     (ROTR(x,6)  ^ ROTR(x,11) ^ ROTR(x,25))
#define SIG0(x)    (ROTR(x,7)  ^ ROTR(x,18) ^ ((x) >> 3))
#define SIG1(x)    (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

#ifdef APG_SHA256_HW_AVAILABLE
extern int  apg_sha256_hw_supported(void);
extern void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64]);

static int hw_ok = -1;

static inline int use_hw(void) {
    if (__builtin_expect(hw_ok < 0, 0))
        hw_ok = apg_sha256_hw_supported() ? 1 : 0;
    return hw_ok;
}
#endif

static void
sha256_transform(sha256_ctx *ctx, const uint8_t data[64])
{
#ifdef APG_SHA256_HW_AVAILABLE
    if (use_hw()) {
        apg_sha256_transform_hw(ctx->state, data);
        return;
    }
#endif
    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = ((uint32_t)data[i*4]     << 24)
             | ((uint32_t)data[i*4 + 1] << 16)
             | ((uint32_t)data[i*4 + 2] <<  8)
             | ((uint32_t)data[i*4 + 3]);
    }
    for (; i < 64; i++)
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];

    a = ctx->state[0]; b = ctx->state[1];
    c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5];
    g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e,f,g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b;
    ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f;
    ctx->state[6] += g; ctx->state[7] += h;
}

void
sha256_init(sha256_ctx *ctx)
{
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

void
sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len)
{
    size_t buf_used = ctx->count % 64;
    size_t buf_free = 64 - buf_used;

    ctx->count += len;

    if (len >= buf_free) {
        memcpy(ctx->buf + buf_used, data, buf_free);
        sha256_transform(ctx, ctx->buf);
        data += buf_free;
        len  -= buf_free;

        while (len >= 64) {
            sha256_transform(ctx, data);
            data += 64;
            len  -= 64;
        }

        buf_used = 0;
    }

    memcpy(ctx->buf + buf_used, data, len);
}

void
sha256_final(sha256_ctx *ctx, uint8_t digest[32])
{
    uint64_t bits = ctx->count * 8;
    uint8_t pad = 0x80;
    sha256_update(ctx, &pad, 1);
    pad = 0x00;
    while (ctx->count % 64 != 56)
        sha256_update(ctx, &pad, 1);

    uint8_t len_bytes[8];
    for (int i = 7; i >= 0; i--) {
        len_bytes[i] = (uint8_t)(bits & 0xff);
        bits >>= 8;
    }
    sha256_update(ctx, len_bytes, 8);

    for (int i = 0; i < 8; i++) {
        digest[i*4]     = (uint8_t)(ctx->state[i] >> 24);
        digest[i*4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i*4 + 2] = (uint8_t)(ctx->state[i] >>  8);
        digest[i*4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

bool
compute_sha256(const char *path, uint8_t digest[32])
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    sha256_ctx ctx;
    sha256_init(&ctx);

    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        sha256_update(&ctx, buf, n);

    fclose(f);
    sha256_final(&ctx, digest);
    return true;
}

void
sha256_hex(const uint8_t digest[32], char *hex)
{
    for (int i = 0; i < 32; i++)
        snprintf(&hex[i * 2], 3, "%02x", digest[i]);
    hex[64] = '\0';
}
