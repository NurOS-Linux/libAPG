// NurOS Ruzen42 2025 apg/util.h
// Last change: Dec 25

#ifndef APG_UTIL_H
#define APG_UTIL_H

enum log_level { INF, ERR, WRN, DBG, FTL };

char *concat(const char *str1, const char *str2);
void log_two(enum log_level level, const char *str1, const char *str2, const FILE *file);

#endif
