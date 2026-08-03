// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file keyring.h
 * @brief Trusted keyring management with trust-chain enforcement
 *        (libsodium, Ed25519).
 *
 * Reads @c *.key files (raw Ed25519 public keys) from the keyring
 * directory. New keys may only be added if they are endorsed by a key
 * already in a trusted keyring, preventing arbitrary key injection.
 */

#include "export.h"

#include <stdbool.h>

/**
 * @brief Opaque handle representing a loaded keyring.
 */
struct keyring;

/**
 * @brief Load all trusted public keys from a keyring directory.
 *
 * Reads all @c *.key files in @p keyring_dir.
 *
 * @param keyring_dir Path to the directory containing the trusted keys.
 * @return Heap-allocated keyring handle, or NULL on failure.
 *         Free with keyring_free().
 */
APG_API struct keyring *keyring_load(const char *keyring_dir);

/**
 * @brief Free a keyring handle and release all associated resources.
 *
 * @param kr Keyring to free. May be NULL.
 */
APG_API void keyring_free(struct keyring *kr);

/**
 * @brief Verify that a package archive's detached signature comes from a
 *        trusted key.
 *
 * @param kr       Loaded keyring of trusted keys.
 * @param pkg_path Path to the package archive.
 * @param sig_path Path to the detached signature file.
 * @return true if the signature is valid and the signing key is in @p kr.
 */
APG_API bool keyring_verify(const struct keyring *kr, const char *pkg_path,
                            const char *sig_path);

/**
 * @brief Add a new public key to the keyring, if endorsed by a trusted key.
 *
 * The new key is only added when its endorsement signature is valid against
 * a key already present in @p trusted. @p key_sig_path is a detached
 * Ed25519 signature of the raw key bytes.
 *
 * @param keyring_dir    Directory to which the new key will be written.
 * @param new_key_path   Path to the public key file to add.
 * @param key_sig_path   Path to the endorsement signature.
 * @param trusted        Keyring of keys permitted to endorse new keys.
 * @return true if the key was verified and added, false otherwise.
 */
APG_API bool keyring_add_key(const char *keyring_dir, const char *new_key_path,
                             const char *key_sig_path,
                             const struct keyring *trusted);
