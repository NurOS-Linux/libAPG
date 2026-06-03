// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

typedef enum {
    VER_OP_ANY,
    VER_OP_EQ,
    VER_OP_NEQ,
    VER_OP_LT,
    VER_OP_LE,
    VER_OP_GT,
    VER_OP_GE,
} ver_op_t;

struct dep_constraint {
    char *name;
    ver_op_t op;
    char *version;  // NULL when op == VER_OP_ANY
};

struct dep_constraint_list {
    struct dep_constraint *items;
    int count;
};

// Parse "libfoo >= 2.0" into a dep_constraint.
// Plain names like "libfoo" yield op=VER_OP_ANY and version=NULL.
struct dep_constraint dep_constraint_parse(const char *str);

// Serialize a constraint back to its string form: "libfoo >= 2.0" or "libfoo".
// Returns a heap-allocated string; caller must free().
char *dep_constraint_to_str(const struct dep_constraint *c);

void dep_constraint_free(struct dep_constraint *c);
void dep_constraint_list_free(struct dep_constraint_list *list);

// Compare two version strings. Returns negative if a < b, 0 if equal, positive if a > b.
// Segments separated by '.' are compared numerically when both are digits.
int ver_compare(const char *a, const char *b);

// Returns true if pkg_version satisfies the constraint described by (op, constraint_version).
bool ver_satisfies(const char *pkg_version, ver_op_t op, const char *constraint_version);
