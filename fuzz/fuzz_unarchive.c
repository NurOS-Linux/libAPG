// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/package.h>
#include <apg/util.h>

static char g_dir[] = "/tmp/apg-fuzz-unarchive-XXXXXX";
static char g_archive[PATH_MAX];
static char g_root[PATH_MAX];
static char g_canary[PATH_MAX];

static void
cleanup(void)
{
    remove_dir_recursive(g_dir);
}

static void
make_path(const char *path)
{
    char buf[PATH_MAX];
    size_t len = strlen(path);
    if (len >= sizeof(buf))
        return;
    memcpy(buf, path, len + 1);

    for (size_t i = 1; i < len; i++)
    {
        if (buf[i] != '/')
            continue;
        buf[i] = '\0';
        mkdir(buf, 0755);
        buf[i] = '/';
    }
    mkdir(buf, 0755);
}

static int
count_entries(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return -1;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            count++;
    }
    closedir(dir);
    return count;
}

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    if (!mkdtemp(g_dir))
        abort();
    snprintf(g_archive, sizeof(g_archive), "%s/input.apg", g_dir);
    snprintf(g_root, sizeof(g_root), "%s/root", g_dir);
    snprintf(g_canary, sizeof(g_canary), "%s%s/canary", g_root, APG_TMP_DIR);
    atexit(cleanup);
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *f = fopen(g_archive, "wb");
    if (!f)
        return 0;
    if (size > 0 && fwrite(data, 1, size, f) != size)
    {
        fclose(f);
        return 0;
    }
    fclose(f);

    remove_dir_recursive(g_root);
    make_path(g_canary);

    struct package *pkg = parse_package(g_archive, g_root);
    package_free(pkg);

    if (count_entries(g_canary) != 0)
        abort();

    return 0;
}
