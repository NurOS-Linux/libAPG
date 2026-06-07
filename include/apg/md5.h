// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file md5.h
 * @brief MD5 hash implementation (streaming and file-level API).
 *
 * On x86-64, aarch64, and riscv64, hardware acceleration is used when the
 * @c APG_MD5_HW_AVAILABLE preprocessor flag is set at build time.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief MD5 hash computation context.
 *
 * Initialise with md5_init(), feed data with md5_update(), and finalise
 * with md5_final().
 */
typedef struct {
    uint32_t state[4]; /**< Intermediate hash state (A, B, C, D). */
    uint32_t count[2]; /**< Total bit count (low word, high word). */
    uint8_t  buf[64];  /**< Partial-block buffer. */
} md5_ctx;

/**
 * @brief Initialise an MD5 context.
 *
 * @param ctx Context to initialise.
 */
void md5_init(md5_ctx *ctx);

/**
 * @brief Feed data into an MD5 context.
 *
 * May be called multiple times; the context accumulates the running state.
 *
 * @param ctx  Context to update.
 * @param data Input bytes.
 * @param len  Number of bytes in @p data.
 */
void md5_update(md5_ctx *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalise the MD5 computation and produce a 16-byte digest.
 *
 * The context must not be used after this call.
 *
 * @param digest Output buffer for the 16-byte digest.
 * @param ctx    Context to finalise.
 */
void md5_final(uint8_t digest[16], md5_ctx *ctx);

/**
 * @brief Compute the MD5 digest of a file.
 *
 * @param path   Path to the file.
 * @param digest Output buffer for the 16-byte digest.
 * @return true on success, false if the file could not be read.
 */
bool compute_md5(const char *path, uint8_t digest[16]);
