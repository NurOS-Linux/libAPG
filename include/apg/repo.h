// NurOS Ruzen42 2025 apg/db.h
// Last change: Dec 31
#pragma once
#ifndef APG_REPO_H
#include <stdbool.h>

enum mirror_type { FTP, HTTP };

struct mirror {
    char *url;
    enum mirror_type type;
};

struct mirror check_mirror(char *mirror_file);
bool download_package(char *name, char *mirror_file, char *dest);
struct package *get_updates(char *mirror_file, char *root);

#endif //APG_REPO_H