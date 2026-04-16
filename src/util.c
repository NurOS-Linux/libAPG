// NurOS Ruzen42 2025 apg/util.c
// Last change: Dec 25

#include <string.h>
#include <stdlib.h>
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
create_dir(const char *path)
{
  struct stat st = {0};

  if (stat(path, &st) == -1) {
    mkdir(path, 0755);
  }
}

char *
concat_dirs(const char *path1, const char *path2)
{
  char *result = concat(path1, path2);
  if (!result) return NULL;
  
  int i = 0, j = 0;

  while (result[i] != '\0') {
    result[j] = result[i];

    if (result[i] == '/') {
      while (result[i] == '/') {
        ++i;
      }
    } else {
      i++;
    }
    j++;
  }

  result[j] = '\0';

  return result;
}
