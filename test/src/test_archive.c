// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <archive.h>
#include <archive_entry.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <apg/archive.h>
#include <apg/package.h>

static char *
join_path(const char *a, const char *b)
{
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", a, b);
    return strdup(buf);
}

static void
rmtree(const char *path)
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
        char *child = join_path(path, entry->d_name);
        if (child)
        {
            rmtree(child);
            free(child);
        }
    }
    closedir(dir);
    rmdir(path);
}

static void
write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

static bool
file_contains(const char *path, const char *expected)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    char buf[256] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    return n == strlen(expected) && memcmp(buf, expected, n) == 0;
}

static char *
mktmp_dir(const char *suffix)
{
    char tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/apg-test-%s-XXXXXX", suffix);
    char *made = mkdtemp(tmpl);
    assert(made);
    return strdup(made);
}

void
test_unarchive_preserves_hardlinks(void)
{
    char *pkg_src = mktmp_dir("archsrc");
    char *arch_dir = mktmp_dir("archdir");
    char *archive_path = join_path(arch_dir, "hardlink.tar.gz");

    char *first_path = join_path(pkg_src, "first.txt");
    write_file(first_path, "shared content");
    char *second_path = join_path(pkg_src, "second.txt");
    assert(link(first_path, second_path) == 0);

    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), "tar -czf '%s' -C '%s' first.txt second.txt",
             archive_path, pkg_src);
    assert(system(cmd) == 0);

    char *root = mktmp_dir("archroot");

    struct package *pkg = package_new();
    assert(pkg);
    pkg->pkg_path = strdup(archive_path);

    assert(unarchive_package_in_root(pkg, root));
    assert(archive_last_error() == NULL);

    char *installed_first = join_path(root, "first.txt");
    char *installed_second = join_path(root, "second.txt");

    struct stat first_st, second_st;
    assert(stat(installed_first, &first_st) == 0);
    assert(stat(installed_second, &second_st) == 0);
    assert(first_st.st_dev == second_st.st_dev);
    assert(first_st.st_ino == second_st.st_ino);
    assert(first_st.st_nlink >= 2);
    assert(file_contains(installed_first, "shared content"));
    assert(file_contains(installed_second, "shared content"));

    free(installed_first);
    free(installed_second);
    package_free(pkg);
    free(first_path);
    free(second_path);
    free(archive_path);
    rmtree(arch_dir);
    rmtree(pkg_src);
    rmtree(root);
    free(arch_dir);
    free(pkg_src);
    free(root);
    printf("test_unarchive_preserves_hardlinks: PASS\n");
}

static void
build_hardlink_escape_archive(const char *archive_path,
                              const char *escape_target)
{
    struct archive *a = archive_write_new();
    assert(archive_write_add_filter_gzip(a) == ARCHIVE_OK);
    assert(archive_write_set_format_ustar(a) == ARCHIVE_OK);
    assert(archive_write_open_filename(a, archive_path) == ARCHIVE_OK);

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, "safe.txt");
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, 5);
    archive_entry_set_perm(entry, 0644);
    assert(archive_write_header(a, entry) == ARCHIVE_OK);
    assert(archive_write_data(a, "hello", 5) == 5);
    archive_entry_free(entry);

    entry = archive_entry_new();
    archive_entry_set_pathname(entry, "evil-link.txt");
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, 0);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_hardlink(entry, escape_target);
    (void)archive_write_header(a, entry);
    archive_entry_free(entry);

    assert(archive_write_close(a) == ARCHIVE_OK);
    archive_write_free(a);
}

void
test_unarchive_rejects_path_traversal_hardlink(void)
{
    char *arch_dir = mktmp_dir("archdir-evil");
    char *archive_path = join_path(arch_dir, "evil.tar.gz");
    build_hardlink_escape_archive(archive_path,
                                  "../apg-hardlink-escape-marker");

    char *root = mktmp_dir("archroot-evil");

    struct package *pkg = package_new();
    assert(pkg);
    pkg->pkg_path = strdup(archive_path);

    assert(!unarchive_package_in_root(pkg, root));

    const char *err = archive_last_error();
    assert(err);
    assert(strstr(err, "unsafe hardlink") != NULL);

    char *escape_marker = join_path("/tmp", "apg-hardlink-escape-marker");
    struct stat st;
    assert(stat(escape_marker, &st) != 0);

    unlink(escape_marker);
    free(escape_marker);
    package_free(pkg);
    free(archive_path);
    rmtree(arch_dir);
    rmtree(root);
    free(arch_dir);
    free(root);
    printf("test_unarchive_rejects_path_traversal_hardlink: PASS\n");
}

void
test_unarchive_rejects_absolute_hardlink(void)
{
    char *arch_dir = mktmp_dir("archdir-abs");
    char *archive_path = join_path(arch_dir, "abs.tar.gz");
    build_hardlink_escape_archive(archive_path, "/tmp/apg-hardlink-abs-marker");

    char *root = mktmp_dir("archroot-abs");

    struct package *pkg = package_new();
    assert(pkg);
    pkg->pkg_path = strdup(archive_path);

    assert(!unarchive_package_in_root(pkg, root));

    const char *err = archive_last_error();
    assert(err);
    assert(strstr(err, "unsafe hardlink") != NULL);

    struct stat st;
    assert(stat("/tmp/apg-hardlink-abs-marker", &st) != 0);

    package_free(pkg);
    free(archive_path);
    rmtree(arch_dir);
    rmtree(root);
    free(arch_dir);
    free(root);
    printf("test_unarchive_rejects_absolute_hardlink: PASS\n");
}

void
test_unarchive_reports_open_failure(void)
{
    char *arch_dir = mktmp_dir("archdir-missing");
    char *archive_path = join_path(arch_dir, "does-not-exist.tar.gz");
    char *root = mktmp_dir("archroot-missing");

    struct package *pkg = package_new();
    assert(pkg);
    pkg->pkg_path = strdup(archive_path);

    assert(!unarchive_package_in_root(pkg, root));

    const char *err = archive_last_error();
    assert(err);
    assert(strstr(err, archive_path) != NULL);

    package_free(pkg);
    free(archive_path);
    rmtree(arch_dir);
    rmtree(root);
    free(arch_dir);
    free(root);
    printf("test_unarchive_reports_open_failure: PASS\n");
}
