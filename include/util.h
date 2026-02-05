// NurOS Ruzen42 2025 apg/util.h
// Last change: Dec 25

#pragma once

enum log_level { INF, ERR, WRN, DBG, FTL };

char *concat(const char *str1, const char *str2);
void log_two(enum log_level level, const char *str1, const char *str2, const FILE *file);

void create_dir(const char *path); 

char *concat_dirs(const char *path1, const char *path2);

