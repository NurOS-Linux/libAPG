<<<<<<< HEAD
// NurOS Ruzen42 2026 apg/db.h
// Last change: Jan 15 
=======
// NurOS Ruzen42 2025 apg/db.h
<<<<<<< HEAD
// Last change: Jan 11 
=======
// Last change: Dec 31
#pragma once
>>>>>>> e6395cd (feat(add db realization))
>>>>>>> 43585c5 (feat(add db realization))

#ifndef APG_DB_H
#define APG_DB_H

#include <lmdb.h>
#include <stdbool.h>

<<<<<<< HEAD
<<<<<<< HEAD
#include "package.h"

MDB_env *init_db(char *db_path);
bool add_package(struct package *pkg, MDB_env *env);
bool remove_package(char *pkg_name, MDB_env *env);
struct package *get_all_packages(MDB_env *env);
struct package get_package_by_name(char *name, MDB_env *env);
=======
=======
=======
MDB_env *init_db_env(const char *db_path);
bool add_package(struct package *pkg, MDB_env *env);
bool remove_package(char *pkg_name, char *db_path);
struct package *get_all_packages(char *db_path);
struct package get_package_by_name(char *name, char *db_path);

>>>>>>> e6395cd (feat(add db realization))
>>>>>>> 43585c5 (feat(add db realization))
bool add_package_in_root(struct package *pkg, char *db_path, char *root_path);
bool remove_package_in_root(char *pkg_name, char *db_path, char *root_path);
struct package *get_all_packages_in_root(char *db_path, char *root_path);
struct package get_package_by_name_in_root(char *name, char *db_path, char *root_path);
>>>>>>> 3257000 (fix)

#endif //APG_DB_H
