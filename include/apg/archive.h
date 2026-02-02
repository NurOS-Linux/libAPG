// NurOS Ruzen42 2025 apg/archive.h
// Last change: Dec 21

#pragma once

#include "package.h"

void unarchive_package(const struct package *pkg, const char *path);
void unarchive_package_in_root(const struct package *pkg, const char *path, const char *root);

