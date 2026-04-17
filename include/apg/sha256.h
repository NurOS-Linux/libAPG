// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 XCubicArnament <xcubicarnament@nuros.org>
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buf[64];
} sha256_ctx;

void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t digest[32]);

bool compute_sha256(const char *path, uint8_t digest[32]);
void sha256_hex(const uint8_t digest[32], char *hex);
