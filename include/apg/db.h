// NurOS Ruzen42 2026 apg/db.h
// Last change: Jan 15 

#ifndef APG_DB_H
#define APG_DB_H

#include <lmdb.h>
#include <stdbool.h>

#include "package.h"

MDB_env *init_db(char *db_path);
bool add_package(struct package *pkg, MDB_env *env);
bool remove_package(char *pkg_name, MDB_env *env);
struct package *get_all_packages(MDB_env *env);
struct package get_package_by_name(char *name, MDB_env *env);

#endif //APG_DB_H
