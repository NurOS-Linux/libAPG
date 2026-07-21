// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file sign.h
 * @brief Package signature creation and verification.
 *
 * Both signing backends are always built in and available side by side:
 * - the plain @c sign_verify / @c sign_file names use **libsodium**
 *   (Ed25519), and are the default choice for new setups.
 * - the @c _gpgme suffixed names use **gpgme** (OpenPGP).
 *
 * There is no build-time or runtime "active backend" switch inside libapg
 * itself; callers decide which pair of functions to invoke. See the keyring
 * API in keyring.h for the matching key management functions.
 */

#include <stdbool.h>

/**
 * @brief Identifies which signing/keyring backend a caller wants to use.
 *
 * Used by callers (e.g. @ref install_policy) to pick between the two
 * backends built into libapg; it is not consulted by sign_verify() /
 * sign_file() / keyring_load() themselves, which are always libsodium.
 */
typedef enum
{
    SIGN_BACKEND_SODIUM = 0, /**< libsodium (Ed25519), the default. */
    SIGN_BACKEND_GPGME,      /**< gpgme (OpenPGP). */
} sign_backend_t;

/**
 * @brief Verify the detached signature of a package archive (libsodium
 *        backend, Ed25519).
 *
 * @p allow_rsa is accepted for interface symmetry with sign_verify_gpgme()
 * but has no effect here.
 *
 * @param pkg_path  Path to the package archive.
 * @param sig_path  Path to the detached signature file.
 * @param allow_rsa Ignored by this backend.
 * @return true if the signature is valid and the signing key is trusted.
 */
bool sign_verify(const char *pkg_path, const char *sig_path, bool allow_rsa);

/**
 * @brief Create a detached signature for a package archive (libsodium
 *        backend, Ed25519).
 *
 * @param pkg_path Path to the package archive to sign.
 * @param sig_path Path where the detached signature will be written.
 * @return true on success, false if signing failed.
 */
bool sign_file(const char *pkg_path, const char *sig_path);

/**
 * @brief Verify the detached signature of a package archive (gpgme backend,
 *        OpenPGP).
 *
 * Only ECC keys (Ed25519, ECDSA) are accepted unless @p allow_rsa is true.
 *
 * @param pkg_path  Path to the package archive.
 * @param sig_path  Path to the detached signature file.
 * @param allow_rsa If true, also accept RSA signatures.
 * @return true if the signature is valid and the signing key is trusted.
 */
bool sign_verify_gpgme(const char *pkg_path, const char *sig_path,
                       bool allow_rsa);

/**
 * @brief Create a detached signature for a package archive (gpgme backend,
 *        OpenPGP).
 *
 * @param pkg_path Path to the package archive to sign.
 * @param sig_path Path where the detached signature will be written.
 * @return true on success, false if signing failed.
 */
bool sign_file_gpgme(const char *pkg_path, const char *sig_path);
