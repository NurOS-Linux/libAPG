// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2025 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../include/util.h"

char *
concat(const char *str1, const char *str2)
{
    int len1 = 0;
    int len2 = 0;
    int i, j;

    while (str1[len1] != '\0')
    {
        len1++;
    }

    while (str2[len2] != '\0')
    {
        len2++;
    }

    char *result = malloc(len1 + len2 + 1);
    if (result == NULL)
    {
        return NULL;
    }

    for (i = 0; i < len1; i++)
    {
        result[i] = str1[i];
    }

    for (j = 0; j < len2; j++, i++)
    {
        result[i] = str2[j];
    }

    result[i] = '\0';

    return result;
}

void
create_dir(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        mkdir(path, 0755);
    }
}

char *
concat_dirs(const char *path1, const char *path2)
{
    char *result = concat(path1, path2);
    if (!result)
        return NULL;

    int i = 0, j = 0;

    while (result[i] != '\0')
    {
        result[j] = result[i];

        if (result[i] == '/')
        {
            while (result[i] == '/')
            {
                ++i;
            }
        }
        else
        {
            i++;
        }
        j++;
    }

    result[j] = '\0';

    return result;
}

static void
collect_recursive(const char *dir, size_t base_len, char ***out, int *count,
                  int *cap)
{
    DIR *d = opendir(dir);
    if (!d)
        return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        size_t len = strlen(dir) + 1 + strlen(entry->d_name) + 1;
        char *abs = malloc(len);
        if (!abs)
            continue;
        snprintf(abs, len, "%s/%s", dir, entry->d_name);

        struct stat st;
        if (stat(abs, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                collect_recursive(abs, base_len, out, count, cap);
            }
            else if (S_ISREG(st.st_mode))
            {
                if (*count == *cap)
                {
                    int new_cap = *cap * 2;
                    char **tmp = realloc(*out, new_cap * sizeof(char *));
                    if (!tmp)
                    {
                        free(abs);
                        continue;
                    }
                    *out = tmp;
                    *cap = new_cap;
                }
                (*out)[(*count)++] = strdup(abs + base_len);
            }
        }
        free(abs);
    }

    closedir(d);
}

char **
collect_files(const char *base, int *count)
{
    *count = 0;
    int cap = 16;
    char **files = malloc(cap * sizeof(char *));
    if (!files)
        return NULL;
    collect_recursive(base, strlen(base), &files, count, &cap);
    return files;
}
