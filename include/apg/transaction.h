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
 * 4. Inspect the plan with trans_plan_count()/trans_plan_at() or conflicts
 * with trans_conflict_count()/trans_conflict_at().
 * 5. Execute with trans_commit().
 * 6. Free with trans_free().
 *
 * trans_commit() may only be called once per transaction. On failure, already
 * applied DB records are rolled back automatically.
 */

#include <stddef.h>
#include <stdbool.h>
#include "config.h"
#include "db.h"
#include "export.h"
#include "package.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Operation type for a single transaction step.
     */
    typedef enum
    {
        TRANS_OP_INSTALL, /**< Install a package. */
        TRANS_OP_REMOVE,  /**< Remove a package. */
        TRANS_OP_UPGRADE, /**< Replace an installed package with a newer
                             version. */
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
        TRANS_ERR_NOT_PREPARED, /**< trans_commit() called without
                                 * trans_prepare().
                                 */
        TRANS_ERR_ALREADY_COMMITTED, /**< trans_commit() called more than once.
                                      */
        TRANS_ERR_INSTALL_FAILED, /**< One or more packages failed to install.
                                   */
        TRANS_ERR_UNSIGNED,       /**< Package rejected by signature policy. */
        TRANS_ERR_HAS_DEPENDENTS, /**< Removal blocked by installed dependents.
                                   */
        TRANS_ERR_FILE_CONFLICT,  /**< File already owned by another package. */
        TRANS_ERR_HELD,           /**< Operation blocked by a held package. */
    } trans_error_t;

    /**
     * @brief One step in the resolved execution plan.
     *
     * Opaque; read fields with trans_step_op(), trans_step_pkg_name(), etc.
     * Owned by the transaction and valid until trans_free().
     */
    struct trans_step;

    /**
     * @brief A detected conflict between two packages.
     *
     * Opaque; read fields with trans_conflict_pkg_name(), etc.
     * Owned by the transaction and valid until trans_free().
     */
    struct trans_conflict;

    /**
     * @brief A file conflict between a package being installed and an existing
     * one.
     *
     * Opaque; read fields with trans_file_conflict_path(), etc.
     * Owned by the transaction and valid until trans_free().
     */
    struct trans_file_conflict;

    /**
     * @brief An operation blocked because the target package is held.
     *
     * Opaque; read fields with trans_held_pkg_name(), etc.
     * Owned by the transaction and valid until trans_free().
     */
    struct trans_held_pkg;

    /**
     * @brief A removal blocked by installed dependents.
     *
     * Opaque; read fields with trans_blocked_remove_pkg_name(), etc.
     * Owned by the transaction and valid until trans_free().
     */
    struct trans_blocked_remove;

    /**
     * @brief Opaque transaction handle.
     */
    struct apg_trans;

    /**
     * @brief Operation this step performs.
     */
    APG_API trans_op_t trans_step_op(const struct trans_step *step);

    /**
     * @brief Name of the package this step applies to.
     */
    APG_API const char *trans_step_pkg_name(const struct trans_step *step);

    /**
     * @brief Version string of the package this step applies to.
     */
    APG_API const char *trans_step_pkg_version(const struct trans_step *step);

    /**
     * @brief True when this step was directly requested by the caller (as
     *        opposed to pulled in as a dependency).
     */
    APG_API bool trans_step_explicit(const struct trans_step *step);

    /**
     * @brief Name of the package being installed or removed.
     */
    APG_API const char *
    trans_conflict_pkg_name(const struct trans_conflict *conflict);

    /**
     * @brief Name of the existing package it conflicts with.
     */
    APG_API const char *
    trans_conflict_conflicts_with(const struct trans_conflict *conflict);

    /**
     * @brief Conflicting file path.
     */
    APG_API const char *
    trans_file_conflict_path(const struct trans_file_conflict *conflict);

    /**
     * @brief Name of the package being installed that claims the file.
     */
    APG_API const char *trans_file_conflict_requested_by(
        const struct trans_file_conflict *conflict);

    /**
     * @brief Name of the currently installed package that owns the file.
     */
    APG_API const char *
    trans_file_conflict_owned_by(const struct trans_file_conflict *conflict);

    /**
     * @brief Name of the held package.
     */
    APG_API const char *trans_held_pkg_name(const struct trans_held_pkg *held);

    /**
     * @brief Operation that was blocked (REMOVE or UPGRADE).
     */
    APG_API trans_op_t trans_held_pkg_op(const struct trans_held_pkg *held);

    /**
     * @brief Name of the package that cannot be removed.
     */
    APG_API const char *
    trans_blocked_remove_pkg_name(const struct trans_blocked_remove *blocked);

    /**
     * @brief Number of dependent packages blocking the removal.
     */
    APG_API int trans_blocked_remove_dependent_count(
        const struct trans_blocked_remove *blocked);

    /**
     * @brief Name of the dependent package at @p index.
     *
     * @param blocked Entry to query.
     * @param index   Index in [0, trans_blocked_remove_dependent_count()).
     */
    APG_API const char *trans_blocked_remove_dependent_at(
        const struct trans_blocked_remove *blocked, int index);

    /**
     * @brief Attach an install policy to a transaction.
     *
     * Must be called before trans_commit(). When @p policy->require_signature
     * is true, commit rejects any package whose @c pkg_path.sig does not verify
     * against the keys in @p policy->keyring_dir (default: @c
     * /etc/apg/trusted.d). Pass NULL to clear a previously set policy.
     *
     * @param trans  Transaction to configure.
     * @param policy Policy to apply, or NULL to disable.
     */
    APG_API void trans_set_policy(struct apg_trans *trans,
                                  const install_policy *policy);

    /**
     * @brief Force resolution of a dependency or virtual (provides) name to
     *        a specific package.
     *
     * Without a preference, trans_prepare() resolves a name with multiple
     * providers to an installed one if any exists, otherwise to whichever
     * provider was added to the transaction first — callers that need the
     * user to pick (e.g. an interactive prompt like pacman's) should call
     * this before trans_prepare(). Calling it again for the same @p name
     * replaces the previous choice.
     *
     * @param trans    Transaction to configure.
     * @param name     Dependency or virtual name to resolve.
     * @param pkg_name Package to resolve @p name to.
     */
    APG_API void trans_prefer_provider(struct apg_trans *trans,
                                       const char *name, const char *pkg_name);

    /**
     * @brief Create a new transaction backed by the given database.
     *
     * @param db Open database handle. The transaction borrows it; the caller
     *           must keep it open for the lifetime of the transaction.
     * @return Heap-allocated transaction, or NULL on allocation failure.
     *         Free with trans_free().
     */
    APG_API struct apg_trans *trans_new(struct db_handle *db);

    /**
     * @brief Free a transaction and all its owned resources.
     *
     * @param trans Transaction to free. May be NULL.
     */
    APG_API void trans_free(struct apg_trans *trans);

    /**
     * @brief Queue a package for installation.
     *
     * The transaction borrows @p pkg — the caller retains ownership and must
     * not free it before trans_free().
     *
     * @param trans Transaction to modify.
     * @param pkg   Package to install.
     * @return @ref TRANS_OK, or an error code.
     */
    APG_API trans_error_t trans_add_install(struct apg_trans *trans,
                                            struct package *pkg);

    /**
     * @brief Queue a package for removal.
     *
     * @param trans    Transaction to modify.
     * @param pkg_name Name of the installed package to remove.
     * @return @ref TRANS_OK, or an error code.
     */
    APG_API trans_error_t trans_add_remove(struct apg_trans *trans,
                                           const char *pkg_name);

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
    APG_API trans_error_t trans_add_upgrade(struct apg_trans *trans,
                                            struct package *pkg);

    /**
     * @brief Resolve dependencies, detect conflicts, and build the execution
     * plan.
     *
     * Must be called before trans_plan_at() or trans_commit(). If conflicts are
     * found, @ref TRANS_ERR_CONFLICT is returned; call trans_conflict_at() to
     * inspect them.
     *
     * @param trans Transaction to prepare.
     * @return @ref TRANS_OK on success, or an error code.
     */
    APG_API trans_error_t trans_prepare(struct apg_trans *trans);

    /**
     * @brief Number of steps in the ordered execution plan after a successful
     *        trans_prepare().
     *
     * @param trans Transaction that has been successfully prepared.
     * @return Number of steps in the plan.
     */
    APG_API size_t trans_plan_count(const struct apg_trans *trans);

    /**
     * @brief Retrieve one step of the ordered execution plan.
     *
     * Owned by the transaction and valid until trans_free().
     *
     * @param trans Transaction that has been successfully prepared.
     * @param index Index in [0, trans_plan_count()).
     * @return Pointer to the step, or NULL if @p index is out of range.
     */
    APG_API const struct trans_step *
    trans_plan_at(const struct apg_trans *trans, size_t index);

    /**
     * @brief Number of conflicts detected by trans_prepare().
     *
     * @param trans Transaction after a trans_prepare() call that returned
     *              @ref TRANS_ERR_CONFLICT.
     * @return Number of conflicts.
     */
    APG_API size_t trans_conflict_count(const struct apg_trans *trans);

    /**
     * @brief Retrieve one conflict detected by trans_prepare().
     *
     * Owned by the transaction and valid until trans_free().
     *
     * @param trans Transaction after a trans_prepare() call that returned
     *              @ref TRANS_ERR_CONFLICT.
     * @param index Index in [0, trans_conflict_count()).
     * @return Pointer to the conflict, or NULL if @p index is out of range.
     */
    APG_API const struct trans_conflict *
    trans_conflict_at(const struct apg_trans *trans, size_t index);

    /**
     * @brief Number of removals blocked by installed dependents.
     *
     * Valid after a trans_prepare() call that returned
     * @ref TRANS_ERR_HAS_DEPENDENTS.
     *
     * @param trans Transaction after trans_prepare().
     * @return Number of blocked removes.
     */
    APG_API size_t trans_blocked_remove_count(const struct apg_trans *trans);

    /**
     * @brief Retrieve one removal blocked by installed dependents.
     *
     * Owned by the transaction and valid until trans_free().
     *
     * @param trans Transaction after trans_prepare().
     * @param index Index in [0, trans_blocked_remove_count()).
     * @return Pointer to the entry, or NULL if @p index is out of range.
     */
    APG_API const struct trans_blocked_remove *
    trans_blocked_remove_at(const struct apg_trans *trans, size_t index);

    /**
     * @brief Number of file conflicts detected by trans_prepare().
     *
     * Valid after a trans_prepare() call that returned
     * @ref TRANS_ERR_FILE_CONFLICT.
     *
     * @param trans Transaction after trans_prepare().
     * @return Number of file conflicts.
     */
    APG_API size_t trans_file_conflict_count(const struct apg_trans *trans);

    /**
     * @brief Retrieve one file conflict detected by trans_prepare().
     *
     * Owned by the transaction and valid until trans_free().
     *
     * @param trans Transaction after trans_prepare().
     * @param index Index in [0, trans_file_conflict_count()).
     * @return Pointer to the entry, or NULL if @p index is out of range.
     */
    APG_API const struct trans_file_conflict *
    trans_file_conflict_at(const struct apg_trans *trans, size_t index);

    /**
     * @brief Number of operations blocked by held packages.
     *
     * Valid after a trans_prepare() call that returned @ref TRANS_ERR_HELD.
     *
     * @param trans Transaction after trans_prepare().
     * @return Number of blocked operations.
     */
    APG_API size_t trans_held_pkg_count(const struct apg_trans *trans);

    /**
     * @brief Retrieve one operation blocked by a held package.
     *
     * Owned by the transaction and valid until trans_free().
     *
     * @param trans Transaction after trans_prepare().
     * @param index Index in [0, trans_held_pkg_count()).
     * @return Pointer to the entry, or NULL if @p index is out of range.
     */
    APG_API const struct trans_held_pkg *
    trans_held_pkg_at(const struct apg_trans *trans, size_t index);

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
    APG_API trans_error_t trans_commit(struct apg_trans *trans,
                                       const char *root_path);

#ifdef __cplusplus
}
#endif
