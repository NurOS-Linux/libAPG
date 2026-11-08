// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/version.h>

static inline char *
fs_join(const char *a, const char *b)
{
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", a, b);
    return strdup(buf);
}

static inline void
fs_rmtree(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        unlink(path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        char *child = fs_join(path, entry->d_name);
        if (child)
        {
            fs_rmtree(child);
            free(child);
        }
    }
    closedir(dir);
    rmdir(path);
}

static inline char *
fs_mktmp(const char *suffix)
{
    char tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/apg-test-%s-XXXXXX", suffix);
    char *made = mkdtemp(tmpl);
    assert(made);
    return strdup(made);
}

static inline void
fs_mkdir_p(const char *path)
{
    char buf[PATH_MAX];
    size_t len = strlen(path);
    assert(len < sizeof(buf));
    memcpy(buf, path, len + 1);

    for (size_t i = 1; i < len; i++)
    {
        if (buf[i] == '/')
        {
            buf[i] = '\0';
            mkdir(buf, 0755);
            buf[i] = '/';
        }
    }
    mkdir(buf, 0755);
}

static inline void
fs_write(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

static inline char *
fs_read(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    char *buf = calloc(1, 65536);
    assert(buf);
    size_t n = fread(buf, 1, 65535, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static inline struct db_handle *
fs_open_db(char *path_out)
{
    strcpy(path_out, "/tmp/apg-test-db-XXXXXX");
    if (!mkdtemp(path_out))
        return NULL;
    return db_open(path_out);
}

static inline void
fs_close_db(struct db_handle *db, const char *path)
{
    db_close(db);
    fs_rmtree(path);
}

static inline struct package *
fs_make_package(const char *name, const char *version, const char *desc,
                const char **deps, int dep_count, const char **provides,
                int provides_count, const char **files, int file_count,
                bool by_hand)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup(name);
    pkg->meta->version = strdup(version);
    if (desc)
        pkg->meta->description = strdup(desc);
    pkg->installed_by_hand = by_hand;

    if (dep_count > 0)
    {
        pkg->meta->dependencies.items =
            calloc(dep_count, sizeof(struct dep_constraint));
        for (int i = 0; i < dep_count; i++)
            pkg->meta->dependencies.items[i] = dep_constraint_parse(deps[i]);
        pkg->meta->dependencies.count = dep_count;
    }
    if (provides_count > 0)
    {
        pkg->meta->provides.items = calloc(provides_count, sizeof(char *));
        for (int i = 0; i < provides_count; i++)
            pkg->meta->provides.items[i] = strdup(provides[i]);
        pkg->meta->provides.count = provides_count;
    }
    if (file_count > 0)
    {
        pkg->package_files.items = calloc(file_count, sizeof(char *));
        for (int i = 0; i < file_count; i++)
            pkg->package_files.items[i] = strdup(files[i]);
        pkg->package_files.count = file_count;
    }
    return pkg;
}

static inline void
fs_free_names(char **names, int count)
{
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

static inline bool
fs_has_name(char **names, int count, const char *name)
{
    for (int i = 0; i < count; i++)
        if (strcmp(names[i], name) == 0)
            return true;
    return false;
}

static inline void
fs_write_tar_entry(FILE *out, const char *name, const char *content)
{
    unsigned char header[512] = {0};
    size_t size = strlen(content);
    snprintf((char *)header, 100, "%s", name);
    snprintf((char *)header + 100, 8, "%07o", 0644);
    snprintf((char *)header + 108, 8, "%07o", 0);
    snprintf((char *)header + 116, 8, "%07o", 0);
    snprintf((char *)header + 124, 12, "%011o", (unsigned)size);
    snprintf((char *)header + 136, 12, "%011o", 0);
    memset(header + 148, ' ', 8);
    header[156] = '0';
    memcpy(header + 257, "ustar", 5);
    memcpy(header + 263, "00", 2);

    unsigned sum = 0;
    for (size_t i = 0; i < sizeof(header); i++)
        sum += header[i];
    snprintf((char *)header + 148, 7, "%06o", sum);
    header[155] = ' ';

    fwrite(header, 1, sizeof(header), out);
    fwrite(content, 1, size, out);
    unsigned char pad[512] = {0};
    fwrite(pad, 1, (512 - size % 512) % 512, out);
}
