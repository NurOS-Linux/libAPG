// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file sign.h
 * @brief Package signature creation and verification (libsodium, Ed25519).
 */

#include "export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Verify the detached signature of a package archive.
     *
     * @param pkg_path  Path to the package archive.
     * @param sig_path  Path to the detached signature file.
     * @param allow_rsa Ignored; kept for interface stability.
     * @return true if the signature is valid and the signing key is trusted.
     */
    APG_API bool sign_verify(const char *pkg_path, const char *sig_path,
                             bool allow_rsa);

    /**
     * @brief Create a detached signature for a package archive.
     *
     * @param pkg_path Path to the package archive to sign.
     * @param sig_path Path where the detached signature will be written.
     * @return true on success, false if signing failed.
     */
    APG_API bool sign_file(const char *pkg_path, const char *sig_path);

    /**
     * @brief Verify the detached signature of a package archive against a
     *        public key file given by path.
     *
     * Like sign_verify(), but reads the public key from @p public_key_path
     * (32 raw bytes) instead of the build-time keys directory.
     *
     * @param pkg_path        Path to the package archive.
     * @param sig_path        Path to the detached signature file.
     * @param public_key_path Path to the public key file.
     * @return true if the signature is valid for that key, false otherwise or
     *         if any argument is NULL or a file cannot be read.
     */
    APG_API bool sign_verify_with_key(const char *pkg_path,
                                      const char *sig_path,
                                      const char *public_key_path);

    /**
     * @brief Create a detached signature for a package archive with a secret
     *        key file given by path.
     *
     * Like sign_file(), but reads the secret key from @p secret_key_path (64
     * raw bytes, as produced by @c crypto_sign_keypair()) instead of the
     * build-time keys directory.
     *
     * @param pkg_path        Path to the package archive to sign.
     * @param sig_path        Path where the detached signature will be written.
     * @param secret_key_path Path to the secret key file.
     * @return true on success, false if signing failed or any argument is
     *         NULL.
     */
    APG_API bool sign_file_with_key(const char *pkg_path, const char *sig_path,
                                    const char *secret_key_path);

#ifdef __cplusplus
}
#endif
