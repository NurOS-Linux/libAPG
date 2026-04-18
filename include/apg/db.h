// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
#pragma once

#include <lmdb.h>
#include <stdbool.h>

#include "package.h"

MDB_env *init_db(const char *db_path);
bool add_package(struct package *pkg, MDB_env *env);
bool remove_package(char *pkg_name, MDB_env *env);
struct package *get_all_packages(MDB_env *env);
struct package get_package_by_name(char *name, MDB_env *env);

