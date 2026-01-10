// NurOS Ruzen42 2025 apg/db.h
// Last change: Jan 11 

#ifndef APG_DB_H
#define APG_DB_H

#include "package.h"
#include <stdbool.h>

bool add_package(struct package *pkg, char *db_path);
bool remove_package(char *pkg_name, char *db_path);
struct package *get_all_packages(char *db_path);
struct package get_package_by_name(char *name, char *db_path);

#endif //APG_DB_H
