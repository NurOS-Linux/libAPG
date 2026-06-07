// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 XCubicArnament <xcubicarnament@nuros.org>
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file sha256.h
 * @brief SHA-256 hash implementation (streaming and file-level API).
 *
 * On x86-64, aarch64, and riscv64, hardware acceleration is used when the
 * @c APG_SHA256_HW_AVAILABLE preprocessor flag is set at build time.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief SHA-256 hash computation context.
 *
 * Initialise with sha256_init(), feed data with sha256_update(), and finalise
 * with sha256_final().
 */
typedef struct {
    uint32_t state[8]; /**< Intermediate hash state (H0–H7). */
    uint64_t count;    /**< Total number of bytes processed. */
    uint8_t  buf[64];  /**< Partial-block buffer. */
} sha256_ctx;

/**
 * @brief Initialise a SHA-256 context.
 *
 * @param ctx Context to initialise.
 */
void sha256_init(sha256_ctx *ctx);

/**
 * @brief Feed data into a SHA-256 context.
 *
 * May be called multiple times; the context accumulates the running state.
 *
 * @param ctx  Context to update.
 * @param data Input bytes.
 * @param len  Number of bytes in @p data.
 */
void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalise the SHA-256 computation and produce a 32-byte digest.
 *
 * The context must not be used after this call.
 *
 * @param ctx    Context to finalise.
 * @param digest Output buffer for the 32-byte digest.
 */
void sha256_final(sha256_ctx *ctx, uint8_t digest[32]);

/**
 * @brief Compute the SHA-256 digest of a file.
 *
 * @param path   Path to the file.
 * @param digest Output buffer for the 32-byte digest.
 * @return true on success, false if the file could not be read.
 */
bool compute_sha256(const char *path, uint8_t digest[32]);

/**
 * @brief Convert a 32-byte SHA-256 digest to a 64-character hex string.
 *
 * @param digest Input 32-byte binary digest.
 * @param hex    Output buffer of at least 65 bytes (64 hex chars + NUL).
 */
void sha256_hex(const uint8_t digest[32], char *hex);
