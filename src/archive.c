// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2025 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <archive.h>
#include <archive_entry.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../include/apg/archive.h"
#include "../include/apg/package.h"
#include "../include/apg/util.h"

#define PATH_MAX 4096

static bool
is_safe_relative_path(const char *path)
{
    if (!path || path[0] == '\0' || path[0] == '/')
        return false;

    const char *p = path;
    while (*p)
    {
        const char *seg_end = strchr(p, '/');
        size_t seg_len = seg_end ? (size_t)(seg_end - p) : strlen(p);
        if (seg_len == 2 && p[0] == '.' && p[1] == '.')
            return false;
        if (!seg_end)
            break;
        p = seg_end + 1;
    }
    return true;
}

static bool
extract_to_dir(const char *archive_path, const char *path_dest)
{
    struct archive_entry *entry;
    char full_path[PATH_MAX];
    char full_link[PATH_MAX];
    bool ok = true;

    struct archive *a = archive_read_new();
    archive_read_support_filter_gzip(a);
    archive_read_support_filter_xz(a);
    archive_read_support_filter_zstd(a);
    archive_read_support_format_tar(a);

    struct archive *ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME |
                                            ARCHIVE_EXTRACT_PERM |
                                            ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                            ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                            ARCHIVE_EXTRACT_UNLINK);

    if (archive_read_open_filename(a, archive_path, 10240) != ARCHIVE_OK)
    {
        archive_read_free(a);
        archive_write_free(ext);
        return false;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        (void)snprintf(full_path, sizeof(full_path), "%s/%s", path_dest,
                       archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, full_path);

        const char *hardlink = archive_entry_hardlink(entry);
        if (hardlink)
        {
            if (!is_safe_relative_path(hardlink) ||
                snprintf(full_link, sizeof(full_link), "%s/%s", path_dest,
                         hardlink) >= (int)sizeof(full_link))
            {
                ok = false;
                continue;
            }
            archive_entry_set_hardlink(entry, full_link);
        }

        if (archive_write_header(ext, entry) != ARCHIVE_OK)
        {
            ok = false;
            continue;
        }

        const void *buff;
        size_t size;
        la_int64_t offset;
        int r;

        while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
               ARCHIVE_OK)
        {
            if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK)
            {
                ok = false;
                break;
            }
        }
        if (r != ARCHIVE_EOF && r != ARCHIVE_OK)
            ok = false;
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return ok;
}

bool
// NOLINTNEXTLINE(misc-use-internal-linkage)
unarchive_package(const struct package *pkg)
{
    return extract_to_dir(pkg->pkg_path, "/");
}

bool
// NOLINTNEXTLINE(misc-use-internal-linkage)
unarchive_package_in_root(const struct package *pkg, const char *root)
{
    return extract_to_dir(pkg->pkg_path, root);
}
