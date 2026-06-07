// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file sign.h
 * @brief Package signature creation and verification.
 *
 * The active signing backend (gpgme or libsodium) is selected at build time.
 * See the keyring API in keyring.h for key management.
 */

#include <stdbool.h>

/**
 * @brief Verify the detached signature of a package archive.
 *
 * With the gpgme backend, only ECC keys (Ed25519, ECDSA) are accepted unless
 * @p allow_rsa is true. With the libsodium backend, @p allow_rsa is ignored.
 *
 * @param pkg_path  Path to the package archive.
 * @param sig_path  Path to the detached signature file.
 * @param allow_rsa If true, also accept RSA signatures (gpgme backend only).
 * @return true if the signature is valid and the signing key is trusted.
 */
bool sign_verify(const char *pkg_path, const char *sig_path, bool allow_rsa);

/**
 * @brief Create a detached signature for a package archive.
 *
 * @param pkg_path Path to the package archive to sign.
 * @param sig_path Path where the detached signature will be written.
 * @return true on success, false if signing failed.
 */
bool sign_file(const char *pkg_path, const char *sig_path);
