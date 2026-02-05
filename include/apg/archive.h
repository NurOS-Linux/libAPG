// NurOS Ruzen42 2026 apg/archive.h
// Last change: Feb 5

#pragma once

#include "package.h"

#include <stdbool.h>

bool unarchive_package(const struct package *pkg, const char *path);
bool unarchive_package_in_root(const struct package *pkg, const char *path, const char *root);

