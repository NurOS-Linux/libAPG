// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <archive.h>
#include <archive_entry.h>
#include <stdlib.h>
#include <string.h>

#include "archive_priv.h"
#include "error_priv.h"
#include "../include/apg/json.h"
#include "../include/apg/package.h"
#include "../include/apg/util.h"

#define METADATA_LIMIT ((la_int64_t)1048576)

static struct package_metadata *
read_metadata_entry(struct archive *a, struct archive_entry *entry,
                    const char *archive_path)
{
    la_int64_t size = archive_entry_size(entry);
    if (size < 0 || size > METADATA_LIMIT)
    {
        apg_set_error("metadata.json in '%s' has an unusable size",
                      archive_path);
        return NULL;
    }

    char *buf = malloc((size_t)size + 1);
    if (!buf)
    {
        apg_set_error("out of memory while reading '%s'", archive_path);
        return NULL;
    }

    la_ssize_t n = archive_read_data(a, buf, (size_t)size);
    struct package_metadata *meta =
        n == size ? package_metadata_from_json(buf, (size_t)size) : NULL;
    free(buf);
    if (!meta)
        apg_set_error("metadata.json in '%s' cannot be read", archive_path);
    return meta;
}

struct package_metadata *
package_read_metadata(const char *archive_path)
{
    if (!archive_path)
        return NULL;
    apg_clear_error();

    struct archive *a = apg_archive_open_read(archive_path);
    if (!a)
        return NULL;

    struct package_metadata *meta = NULL;
    bool found = false;
    struct archive_entry *entry;
    int r;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        const char *name = archive_entry_pathname(entry);
        if (!name || !apg_archive_is_metadata(name))
            continue;
        found = true;
        meta = read_metadata_entry(a, entry, archive_path);
        break;
    }

    if (!found)
    {
        if (r == ARCHIVE_EOF)
            apg_set_error("'%s' has no metadata.json", archive_path);
        else
            apg_set_error("failed to read archive '%s': %s", archive_path,
                          archive_error_string(a));
    }
    archive_read_free(a);
    return meta;
}

bool
package_list_files(const char *archive_path, char ***files, int *count)
{
    if (!archive_path || !files || !count)
        return false;
    *files = NULL;
    *count = 0;
    apg_clear_error();

    struct archive *a = apg_archive_open_read(archive_path);
    if (!a)
        return false;

    char **items = NULL;
    int used = 0;
    int cap = 0;
    bool ok = true;
    struct archive_entry *entry;
    int r;
    while (ok && (r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        const char *name = archive_entry_pathname(entry);
        const char *rel = name ? apg_archive_data_relative(name) : NULL;
        if (!rel || !*rel || archive_entry_filetype(entry) == AE_IFDIR)
            continue;
        if (!apg_archive_is_safe_relative(rel))
        {
            apg_set_error("'%s' contains an unsafe path '%s'", archive_path,
                          name);
            ok = false;
            break;
        }

        if (used == cap)
        {
            int new_cap = cap ? cap * 2 : 16;
            char **tmp = realloc(items, (size_t)new_cap * sizeof(*tmp));
            if (!tmp)
            {
                apg_set_error("out of memory while reading '%s'", archive_path);
                ok = false;
                break;
            }
            items = tmp;
            cap = new_cap;
        }
        items[used] = concat("/", rel);
        if (!items[used])
        {
            apg_set_error("out of memory while reading '%s'", archive_path);
            ok = false;
            break;
        }
        used++;
    }

    if (ok && r != ARCHIVE_EOF)
    {
        apg_set_error("failed to read archive '%s': %s", archive_path,
                      archive_error_string(a));
        ok = false;
    }
    archive_read_free(a);

    if (!ok)
    {
        for (int i = 0; i < used; i++)
            free(items[i]);
        free(items);
        return false;
    }
    *files = items;
    *count = used;
    return true;
}
