// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file transaction.h
 * @brief Atomic multi-package install/remove transactions.
 *
 * A transaction follows a strict lifecycle:
 * 1. Create with trans_new().
 * 2. Queue operations with trans_add_install() / trans_add_remove().
 * 3. Prepare (resolve deps, detect conflicts) with trans_prepare().
 * 4. Inspect the plan with trans_get_plan() or conflicts with
 * trans_get_conflicts().
 * 5. Execute with trans_commit().
 * 6. Free with trans_free().
 *
 * trans_commit() may only be called once per transaction. On failure, already
 * applied DB records are rolled back automatically.
 */

#include <stddef.h>
#include <stdbool.h>
#include "package.h"
#include "db.h"
#include "config.h"

/**
 * @brief Operation type for a single transaction step.
 */
typedef enum
{
    TRANS_OP_INSTALL, /**< Install a package. */
    TRANS_OP_REMOVE,  /**< Remove a package. */
    TRANS_OP_UPGRADE, /**< Replace an installed package with a newer version. */
} trans_op_t;

/**
 * @brief Error codes returned by transaction functions.
 */
typedef enum
{
    TRANS_OK = 0,           /**< Success. */
    TRANS_ERR_NOMEM,        /**< Memory allocation failure. */
    TRANS_ERR_CONFLICT,     /**< One or more package conflicts detected. */
    TRANS_ERR_MISSING_DEP,  /**< A required dependency is not available. */
    TRANS_ERR_CYCLE,        /**< Circular dependency detected. */
    TRANS_ERR_NOT_PREPARED, /**< trans_commit() called without trans_prepare().
                             */
    TRANS_ERR_ALREADY_COMMITTED, /**< trans_commit() called more than once. */
    TRANS_ERR_INSTALL_FAILED,    /**< One or more packages failed to install. */
    TRANS_ERR_UNSIGNED,          /**< Package rejected by signature policy. */
    TRANS_ERR_HAS_DEPENDENTS, /**< Removal blocked by installed dependents. */
    TRANS_ERR_FILE_CONFLICT,  /**< File already owned by another package. */
} trans_error_t;

/**
 * @brief One step in the resolved execution plan.
 *
 * The array is owned by the transaction and valid until trans_free().
 */
struct trans_step
{
    trans_op_t op;     /**< Operation to perform. */
    char *pkg_name;    /**< Package name. */
    char *pkg_version; /**< Package version string. */
    bool explicit;     /**< True when directly requested by the caller. */
};

/**
 * @brief A detected conflict between two packages.
 *
 * The array is owned by the transaction and valid until trans_free().
 */
struct trans_conflict
{
    char *pkg_name;       /**< Package being installed or removed. */
    char *conflicts_with; /**< Existing package that conflicts with it. */
};

/**
 * @brief A file conflict between a package being installed and an existing one.
 *
 * The array is owned by the transaction and valid until trans_free().
 */
struct trans_file_conflict
{
    char *path;         /**< Conflicting file path. */
    char *requested_by; /**< Package being installed that claims the file. */
    char *owned_by;     /**< Currently installed package that owns the file. */
};

/**
 * @brief A removal blocked by installed dependents.
 *
 * The array is owned by the transaction and valid until trans_free().
 */
struct trans_blocked_remove
{
    char *pkg_name;    /**< Package that cannot be removed. */
    char **dependents; /**< Names of packages that depend on it. */
    int dependent_count;
};

/**
 * @brief Opaque transaction handle.
 */
struct apg_trans;

/**
 * @brief Attach an install policy to a transaction.
 *
 * Must be called before trans_commit(). When @p policy->require_signature is
 * true, commit rejects any package whose @c <pkg_path>.sig does not verify
 * against the keys in @p policy->keyring_dir (default: @c /etc/apg/trusted.d).
 * Pass NULL to clear a previously set policy.
 *
 * @param trans  Transaction to configure.
 * @param policy Policy to apply, or NULL to disable.
 */
void trans_set_policy(struct apg_trans *trans, const install_policy *policy);

/**
 * @brief Create a new transaction backed by the given database.
 *
 * @param db Open database handle. The transaction borrows it; the caller
 *           must keep it open for the lifetime of the transaction.
 * @return Heap-allocated transaction, or NULL on allocation failure.
 *         Free with trans_free().
 */
struct apg_trans *trans_new(struct db_handle *db);

/**
 * @brief Free a transaction and all its owned resources.
 *
 * @param trans Transaction to free. May be NULL.
 */
void trans_free(struct apg_trans *trans);

/**
 * @brief Queue a package for installation.
 *
 * The transaction borrows @p pkg — the caller retains ownership and must not
 * free it before trans_free().
 *
 * @param trans Transaction to modify.
 * @param pkg   Package to install.
 * @return @ref TRANS_OK, or an error code.
 */
trans_error_t trans_add_install(struct apg_trans *trans, struct package *pkg);

/**
 * @brief Queue a package for removal.
 *
 * @param trans    Transaction to modify.
 * @param pkg_name Name of the installed package to remove.
 * @return @ref TRANS_OK, or an error code.
 */
trans_error_t trans_add_remove(struct apg_trans *trans, const char *pkg_name);

/**
 * @brief Queue a package for upgrade.
 *
 * The installed package with the same name as @p pkg is replaced in the
 * database and on disk. Files removed between versions are not cleaned up —
 * only the new version's files are written. The transaction borrows @p pkg;
 * the caller retains ownership.
 *
 * @param trans Transaction to modify.
 * @param pkg   New version of the package to install.
 * @return @ref TRANS_OK, or an error code.
 */
trans_error_t trans_add_upgrade(struct apg_trans *trans, struct package *pkg);

/**
 * @brief Resolve dependencies, detect conflicts, and build the execution plan.
 *
 * Must be called before trans_get_plan() or trans_commit(). If conflicts are
 * found, @ref TRANS_ERR_CONFLICT is returned; call trans_get_conflicts() to
 * inspect them.
 *
 * @param trans Transaction to prepare.
 * @return @ref TRANS_OK on success, or an error code.
 */
trans_error_t trans_prepare(struct apg_trans *trans);

/**
 * @brief Retrieve the ordered execution plan after a successful
 * trans_prepare().
 *
 * The returned array is owned by the transaction and valid until trans_free().
 *
 * @param trans Transaction that has been successfully prepared.
 * @param count Output parameter set to the number of steps.
 * @return Pointer to the first step, or NULL if the plan is empty.
 */
const struct trans_step *trans_get_plan(const struct apg_trans *trans,
                                        size_t *count);

/**
 * @brief Retrieve the list of conflicts detected by trans_prepare().
 *
 * The returned array is owned by the transaction and valid until trans_free().
 *
 * @param trans Transaction after a trans_prepare() call that returned
 *              @ref TRANS_ERR_CONFLICT.
 * @param count Output parameter set to the number of conflicts.
 * @return Pointer to the first conflict, or NULL if none.
 */
const struct trans_conflict *trans_get_conflicts(const struct apg_trans *trans,
                                                 size_t *count);

/**
 * @brief Retrieve removals blocked by installed dependents.
 *
 * Valid after a trans_prepare() call that returned
 * @ref TRANS_ERR_HAS_DEPENDENTS. The returned array is owned by the
 * transaction and valid until trans_free().
 *
 * @param trans Transaction after trans_prepare().
 * @param count Output parameter set to the number of blocked removes.
 * @return Pointer to the first entry, or NULL if none.
 */
const struct trans_blocked_remove *
trans_get_blocked_removes(const struct apg_trans *trans, size_t *count);

/**
 * @brief Retrieve file conflicts detected by trans_prepare().
 *
 * Valid after a trans_prepare() call that returned
 * @ref TRANS_ERR_FILE_CONFLICT. The returned array is owned by the transaction
 * and valid until trans_free().
 *
 * @param trans Transaction after trans_prepare().
 * @param count Output parameter set to the number of conflicts.
 * @return Pointer to the first entry, or NULL if none.
 */
const struct trans_file_conflict *
trans_get_file_conflicts(const struct apg_trans *trans, size_t *count);

/**
 * @brief Execute the prepared plan.
 *
 * Must be called at most once. On failure, DB records for already-committed
 * installs are rolled back automatically.
 *
 * @param trans     Transaction that has been successfully prepared.
 * @param root_path Filesystem root for installation (e.g. @c "/").
 * @return @ref TRANS_OK on success, or an error code.
 */
trans_error_t trans_commit(struct apg_trans *trans, const char *root_path);
