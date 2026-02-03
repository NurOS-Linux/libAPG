// NurOS Ruzen42 2025 apg/util.c
// Last change: Dec 25

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <iron/logger_file.h>
#include <iron/logger.h>
#include <sys/stat.h>

#include "../include/util.h"


char *
concat(const char *str1, const char *str2)
{
    int len1 = 0;
    int len2 = 0;
    int i, j;

    while (str1[len1] != '\0') {
        len1++;
    }

    while (str2[len2] != '\0') {
        len2++;
    }

    char *result = malloc(len1 + len2 + 1);
    if (result == NULL) {
        return NULL;
    }

    for (i = 0; i < len1; i++) {
        result[i] = str1[i];
    }

    for (j = 0; j < len2; j++, i++) {
        result[i] = str2[j];
    }

    result[i] = '\0';

    return result;
}

void
log_two(const enum log_level level, const char *str1, const char *str2, const FILE *file)
{
    char *full_err = concat(str1, str2);

    switch (level) {
        case INF:
            log_info_to_file(full_err, file);
            break;
        case ERR:
            log_error_to_file(full_err, file);
            break;
        case WRN:
            log_warn_to_file(full_err, file);
            break;
        case DBG:
            log_debug_to_file(full_err, file);
            break;
        case FTL:
            log_fatal_to_file(full_err, file);
            break;
        default:
            log_info(full_err);
            break;
    }

    free(full_err);
}

void
create_directory(const char *path)
{
  struct stat stats;

  if (stat(path, &stats) == 0) {
    if (S_ISDIR(stats.st_mode)) {
      return;
    } else {

    }
    } else {
    }
}
