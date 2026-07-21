// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file keyring.h
 * @brief Trusted keyring management with trust-chain enforcement.
 *
 * Both backends are always built in and available side by side; callers
 * pick which one to use:
 * - **libsodium** (plain @c keyring_* names, the default choice for new
 *   setups): reads @c *.key files (raw Ed25519 public keys) from the
 *   keyring directory.
 * - **gpgme** (@c _gpgme suffixed names): treats the keyring directory as a
 *   GnuPG homedir.
 *
 * The two backends use distinct opaque handle types (struct keyring vs.
 * struct keyring_gpgme) and are not interchangeable: load and verify with
 * the same backend's functions.
 *
 * New keys may only be added if they are endorsed by a key already in a
 * trusted keyring, preventing arbitrary key injection.
 */

#include <stdbool.h>

/**
 * @brief Opaque handle representing a loaded keyring (libsodium backend).
 */
struct keyring;

/**
 * @brief Load all trusted public keys from a keyring directory (libsodium
 *        backend).
 *
 * Reads all @c *.key files in @p keyring_dir.
 *
 * @param keyring_dir Path to the directory containing the trusted keys.
 * @return Heap-allocated keyring handle, or NULL on failure.
 *         Free with keyring_free().
 */
struct keyring *keyring_load(const char *keyring_dir);

/**
 * @brief Free a keyring handle and release all associated resources
 *        (libsodium backend).
 *
 * @param kr Keyring to free. May be NULL.
 */
void keyring_free(struct keyring *kr);

/**
 * @brief Verify that a package archive's detached signature comes from a
 *        trusted key (libsodium backend).
 *
 * @param kr       Loaded keyring of trusted keys.
 * @param pkg_path Path to the package archive.
 * @param sig_path Path to the detached signature file.
 * @return true if the signature is valid and the signing key is in @p kr.
 */
bool keyring_verify(const struct keyring *kr, const char *pkg_path,
                    const char *sig_path);

/**
 * @brief Add a new public key to the keyring, if endorsed by a trusted key
 *        (libsodium backend).
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
bool keyring_add_key(const char *keyring_dir, const char *new_key_path,
                     const char *key_sig_path, const struct keyring *trusted);

/**
 * @brief Opaque handle representing a loaded keyring (gpgme backend).
 */
struct keyring_gpgme;

/**
 * @brief Load a keyring (gpgme backend).
 *
 * @p keyring_dir is used as the GnuPG homedir.
 *
 * @param keyring_dir Path to the GnuPG homedir containing the trusted keys.
 * @return Heap-allocated keyring handle, or NULL on failure.
 *         Free with keyring_free_gpgme().
 */
struct keyring_gpgme *keyring_load_gpgme(const char *keyring_dir);

/**
 * @brief Free a keyring handle and release all associated resources
 *        (gpgme backend).
 *
 * @param kr Keyring to free. May be NULL.
 */
void keyring_free_gpgme(struct keyring_gpgme *kr);

/**
 * @brief Verify that a package archive's detached signature comes from a
 *        trusted key (gpgme backend).
 *
 * @param kr       Loaded keyring of trusted keys.
 * @param pkg_path Path to the package archive.
 * @param sig_path Path to the detached signature file.
 * @return true if the signature is valid and the signing key is in @p kr.
 */
bool keyring_verify_gpgme(const struct keyring_gpgme *kr, const char *pkg_path,
                          const char *sig_path);

/**
 * @brief Add a new public key to the keyring, if endorsed by a trusted key
 *        (gpgme backend).
 *
 * @p key_sig_path is unused; certifications are embedded in the key
 * packet and verified via gpgme's trust model after import.
 *
 * @param keyring_dir    Directory to which the new key will be written.
 * @param new_key_path   Path to the public key file to add.
 * @param key_sig_path   Unused by this backend.
 * @param trusted        Keyring of keys permitted to endorse new keys.
 * @return true if the key was verified and added, false otherwise.
 */
bool keyring_add_key_gpgme(const char *keyring_dir, const char *new_key_path,
                           const char *key_sig_path,
                           const struct keyring_gpgme *trusted);
