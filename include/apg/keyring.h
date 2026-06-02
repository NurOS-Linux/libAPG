// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

struct keyring;

// Load all trusted public keys from keyring_dir (*.key files for sodium,
// gpgme homedir for pgp). Returns NULL on failure.
struct keyring *keyring_load(const char *keyring_dir);
void keyring_free(struct keyring *kr);

// Verify that pkg_path's detached signature (sig_path) was made by any
// key in the keyring. Both backends require the signing key to be trusted.
bool keyring_verify(const struct keyring *kr, const char *pkg_path,
                    const char *sig_path);

// Add new_key_path to keyring_dir only if endorsed by a key in trusted.
// sodium: key_sig_path is a detached Ed25519 signature of the key bytes.
// pgp:    key_sig_path is unused — certifications are embedded in the key
//         packet and verified via gpgme's trust model after import.
bool keyring_add_key(const char *keyring_dir, const char *new_key_path,
                     const char *key_sig_path, const struct keyring *trusted);
