// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file config.h
 * @brief Install-time security policy passed to trans_set_policy().
 */

#include <stdbool.h>

/**
 * @brief Install-time security policy.
 *
 * When @p require_signature is true, @c trans_commit() refuses to install
 * packages whose detached signature at @c pkg_path.sig cannot be verified
 * against the keys in @p keyring_dir.
 */
typedef struct
{
    bool require_signature; /**< Reject unsigned packages. */
    char *
        keyring_dir; /**< Trusted key directory; NULL → @c /etc/apg/trusted.d */
} install_policy;
