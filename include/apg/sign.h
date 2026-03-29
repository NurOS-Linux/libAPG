// NurOS Ruzen42 2026 apg/sign.h
// Last change: Feb 5

#pragma once

#include <stdbool.h>

bool sign_file(const char *pkg_path);
bool sign_file_by_key(const char *path, const char *sig_path, const unsigned char *public_key);

