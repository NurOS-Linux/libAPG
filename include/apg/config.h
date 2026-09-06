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
 *
 * When @p skip_dependency_check is true, @c trans_prepare() plans installs
 * exactly as added by @c trans_add_install(), in that order, without
 * resolving or requiring their declared dependencies to be present. This
 * does not affect upgrades, removals, or the file-conflict / installed-break
 * checks, which still run as usual.
 */
typedef struct
{
    bool require_signature; /**< Reject unsigned packages. */
    char *
        keyring_dir; /**< Trusted key directory; NULL → @c /etc/apg/trusted.d */
    bool skip_dependency_check; /**< Plan installs as given, ignoring their
                                 * declared dependencies. */
} install_policy;
