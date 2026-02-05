// NurOS Ruzen42 2025 apg/archive.c
// Last change: Dec 31

#include <archive.h>
#include <archive_entry.h>
#include <stdbool.h>
#include <stdio.h>
#include "../include/util.h"
#include "../include/apg/package.h"

#define PATH_MAX 256

const char *log_file_path = "/var/log/apg.log";

bool
extract_to_dir(const char *archive_path, const char *path_dest)
{
    struct archive_entry *entry;
    char full_path[PATH_MAX];
    FILE *log_file = fopen(log_file_path, "a");

    struct archive *a = archive_read_new();
    archive_read_support_filter_xz(a);
    archive_read_support_format_tar(a);

    struct archive *ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);

    if (archive_read_open_filename(a, archive_path, 10240) != ARCHIVE_OK)
        return false;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path_dest, archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, full_path);

        int r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            log_two(ERR, "Header error:", archive_error_string(ext), log_file);
        } else {
            const void *buff;
            size_t size;
            la_int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    log_two(ERR, "Error while writing data: ", archive_error_string(ext), log_file);
                    break;
                }
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return true;
}

bool
unarchive_package(const struct package *pkg, const char *path)
{
    if (!extract_to_dir(pkg->pkg_path, path)) {
        log_two(ERR, "Failed to extract package into: ", (char*)path, stdout);
        return false;
    }
    log_two(WRN, "Package extracted successfully into: ", (char*)path, stdout);
    return true;
}

