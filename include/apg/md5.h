// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t  buf[64];
} md5_ctx;

void md5_init(md5_ctx *ctx);
void md5_update(md5_ctx *ctx, const uint8_t *data, size_t len);
void md5_final(uint8_t digest[16], md5_ctx *ctx);

bool compute_md5(const char *path, uint8_t digest[16]);
