// NurOS Ruzen42 2025 apg/db.h
// Last change: Dec 30

#ifndef APG_DB_H
#define APG_DB_H

#include <apg/package.h>
#include <stdbool.h>

bool add_package(struct package *pkg, char *db_path);
bool remove_package(char *pkg_name, char *db_path);
struct package *get_all_packages(char *db_path);
struct package get_package_by_name(char *name, char *db_path);

#endif //LIBAPG_DB_H