// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file config.h
 * @brief Runtime configuration parsed from @c /etc/apg.conf.
 */

#include <stdbool.h>

/**
 * @brief Install-time security policy.
 *
 * When @p require_signature is true, @c trans_commit() refuses to install
 * packages whose detached signature at @c <pkg_path>.sig cannot be verified
 * against the keys in @p keyring_dir.
 */
typedef struct
{
    bool require_signature; /**< Reject unsigned packages. */
    char *
        keyring_dir; /**< Trusted key directory; NULL → @c /etc/apg/trusted.d */
} install_policy;

/**
 * @brief Transport protocol used to fetch packages from a repository.
 */
typedef enum
{
    HTTP,  /**< Fetch via HTTP. */
    FTP,   /**< Fetch via FTP. */
    RSYNC, /**< Fetch via rsync. */
} repo_type;

/**
 * @brief A single package repository entry.
 */
typedef struct
{
    const char *url; /**< Base URL of the repository. */
    repo_type type;  /**< Transport protocol. */
} repo;

/**
 * @brief Global runtime configuration.
 *
 * Populated by parse_config() and applied globally with set_config().
 * Free with config_free().
 */
typedef struct
{
    int db_size;    /**< Maximum size of the package database in bytes. */
    char *tmp_dir;  /**< Temporary directory used during package operations. */
    int repo_count; /**< Number of entries in @p repos. */
    repo *repos;    /**< Array of repository descriptors. */
    install_policy policy; /**< Install-time security policy. */
} config;

/**
 * @brief Parse the configuration file at the given path.
 *
 * @param path Path to the configuration file (typically @c /etc/apg.conf).
 * @return Heap-allocated config on success, NULL on parse failure.
 *         Free with config_free().
 */
config *parse_config(char *path);

/**
 * @brief Install a parsed configuration as the process-wide active config.
 *
 * @param cfg Configuration to activate. Ownership is transferred.
 */
void set_config(config *cfg);

/**
 * @brief Free a config and all its owned resources.
 *
 * @param cfg Configuration to free. May be NULL.
 */
void config_free(config *cfg);
