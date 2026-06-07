// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file version.h
 * @brief Version string comparison and dependency constraint handling.
 */

#include <stdbool.h>

/**
 * @brief Version comparison operator used in dependency constraints.
 */
typedef enum {
    VER_OP_ANY, /**< No version constraint — any version satisfies. */
    VER_OP_EQ,  /**< Exact version match. */
    VER_OP_NEQ, /**< Any version except the specified one. */
    VER_OP_LT,  /**< Strictly older than the specified version. */
    VER_OP_LE,  /**< Equal to or older than the specified version. */
    VER_OP_GT,  /**< Strictly newer than the specified version. */
    VER_OP_GE,  /**< Equal to or newer than the specified version. */
} ver_op_t;

/**
 * @brief A single dependency constraint.
 *
 * Represents a requirement such as @c "libfoo >= 2.0".
 * When @p op is @ref VER_OP_ANY, @p version is NULL.
 *
 * Free the heap-allocated fields with dep_constraint_free().
 */
struct dep_constraint {
    char *name;    /**< Required package name. Heap-allocated. */
    ver_op_t op;   /**< Comparison operator. */
    char *version; /**< Required version string. NULL when op == VER_OP_ANY. */
};

/**
 * @brief An owned, growable list of dependency constraints.
 */
struct dep_constraint_list {
    struct dep_constraint *items; /**< Array of constraints. */
    int count;                    /**< Number of elements. */
};

/**
 * @brief Parse a dependency string into a constraint.
 *
 * Plain names like @c "libfoo" yield @c op=VER_OP_ANY and @c version=NULL.
 * Versioned strings like @c "libfoo >= 2.0" are parsed fully.
 *
 * @param str Null-terminated dependency string.
 * @return Parsed constraint with heap-allocated fields. Caller owns them;
 *         free with dep_constraint_free().
 */
struct dep_constraint dep_constraint_parse(const char *str);

/**
 * @brief Serialize a constraint to its canonical string form.
 *
 * Produces @c "libfoo >= 2.0" or @c "libfoo" depending on the operator.
 *
 * @param c Constraint to serialize.
 * @return Heap-allocated string; caller must free().
 */
char *dep_constraint_to_str(const struct dep_constraint *c);

/**
 * @brief Release the heap-allocated fields of a dep_constraint.
 *
 * @param c Constraint whose fields should be freed. The struct itself is
 *          not freed (it is typically stack-allocated).
 */
void dep_constraint_free(struct dep_constraint *c);

/**
 * @brief Release the heap-allocated fields of a dep_constraint_list.
 *
 * @param list List whose contents should be freed.
 */
void dep_constraint_list_free(struct dep_constraint_list *list);

/**
 * @brief Compare two version strings.
 *
 * Segments separated by @c '.' are compared numerically when both consist
 * entirely of digits, and lexicographically otherwise.
 *
 * @param a First version string.
 * @param b Second version string.
 * @return Negative if @p a < @p b, zero if equal, positive if @p a > @p b.
 */
int ver_compare(const char *a, const char *b);

/**
 * @brief Test whether a package version satisfies a constraint.
 *
 * @param pkg_version        Installed version string.
 * @param op                 Comparison operator from the constraint.
 * @param constraint_version Required version string. May be NULL when
 *                           @p op is @ref VER_OP_ANY.
 * @return true if the installed version satisfies the constraint.
 */
bool ver_satisfies(const char *pkg_version, ver_op_t op, const char *constraint_version);
