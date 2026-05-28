// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include "package.h"

typedef enum {
    DB_OP_ADD,
    DB_OP_REMOVE,
} db_op_t;

typedef void (*db_hook_fn)(db_op_t op, const char *pkg_name, void *userdata);

struct db_hooks {
    db_hook_fn pre;
    db_hook_fn post;
    void *userdata;
};

struct db_handle;

// Open for reading and writing. Returns NULL on failure.
struct db_handle *db_open(const char *path);

// Open read-only: db_add/db_remove will return false.
struct db_handle *db_open_readonly(const char *path);

void db_close(struct db_handle *db);

// Register pre/post callbacks invoked around every write operation.
void db_set_hooks(struct db_handle *db, const struct db_hooks *hooks);

bool db_add(struct db_handle *db, struct package *pkg);
bool db_remove(struct db_handle *db, const char *pkg_name);
struct package *db_get(struct db_handle *db, const char *name);
struct package **db_list(struct db_handle *db, int *count);
