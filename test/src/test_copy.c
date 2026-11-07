// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/copy.h>
#include <apg/error.h>
#include <apg/util.h>

#include "fs_helpers.h"

static mode_t
mode_of(const char *path)
{
    struct stat st;
    assert(stat(path, &st) == 0);
    return st.st_mode & 07777;
}

static bool
content_is(const char *path, const char *expected)
{
    char *got = fs_read(path);
    bool same = got && strcmp(got, expected) == 0;
    free(got);
    return same;
}

void
test_copy_file_preserves_content_and_mode(void)
{
    char *dir = fs_mktmp("cpfile");
    char *src = fs_join(dir, "src");
    char *dst = fs_join(dir, "dst");

    fs_write(src, "payload");
    assert(chmod(src, 0750) == 0);
    assert(copy_file(src, dst));
    assert(content_is(dst, "payload"));
    assert(mode_of(dst) == 0750);

    fs_write(src, "second");
    assert(copy_file(src, dst));
    assert(content_is(dst, "second"));

    fs_write(src, "");
    assert(copy_file(src, dst));
    assert(content_is(dst, ""));

    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_file_preserves_content_and_mode: PASS\n");
}

void
test_copy_file_large_file(void)
{
    char *dir = fs_mktmp("cpbig");
    char *src = fs_join(dir, "src");
    char *dst = fs_join(dir, "dst");

    FILE *f = fopen(src, "wb");
    assert(f);
    for (int i = 0; i < 3000; i++)
        fprintf(f, "line-%05d-padding-padding-padding\n", i);
    fclose(f);

    assert(copy_file(src, dst));
    char *a = fs_read(src);
    char *b = fs_read(dst);
    assert(a && b && strcmp(a, b) == 0);
    free(a);
    free(b);

    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_file_large_file: PASS\n");
}

void
test_copy_file_reports_failures(void)
{
    char *dir = fs_mktmp("cpfail");
    char *src = fs_join(dir, "src");
    char *missing = fs_join(dir, "missing");
    char *bad_dst = fs_join(dir, "nodir/dst");
    char *subdir = fs_join(dir, "subdir");

    fs_write(src, "x");
    assert(mkdir(subdir, 0755) == 0);

    assert(!copy_file(missing, src));
    assert(apg_last_error() && strstr(apg_last_error(), "missing"));

    assert(!copy_file(src, bad_dst));
    assert(apg_last_error() && strstr(apg_last_error(), "nodir"));

    assert(!copy_file(subdir, src));
    assert(apg_last_error() && *apg_last_error());

    free(src);
    free(missing);
    free(bad_dst);
    free(subdir);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_file_reports_failures: PASS\n");
}

void
test_copy_dir_copies_tree_with_modes_and_links(void)
{
    char *dir = fs_mktmp("cpdir");
    char *src = fs_join(dir, "src");
    char *dst = fs_join(dir, "dst");
    char *nested = fs_join(src, "a/b");
    fs_mkdir_p(nested);

    char *f1 = fs_join(src, "top.txt");
    char *f2 = fs_join(nested, "deep.sh");
    char *lnk = fs_join(src, "link");
    char *dangling = fs_join(src, "dangling");
    fs_write(f1, "top");
    fs_write(f2, "deep");
    assert(chmod(f2, 0755) == 0);
    char *private_dir = fs_join(src, "a");
    assert(chmod(private_dir, 0700) == 0);
    assert(symlink("top.txt", lnk) == 0);
    assert(symlink("/nonexistent/target", dangling) == 0);

    assert(copy_dir(src, dst));

    char *d1 = fs_join(dst, "top.txt");
    char *d2 = fs_join(dst, "a/b/deep.sh");
    char *dl = fs_join(dst, "link");
    char *dd = fs_join(dst, "dangling");
    char *da = fs_join(dst, "a");
    assert(content_is(d1, "top"));
    assert(content_is(d2, "deep"));
    assert(mode_of(d2) == 0755);
    assert(mode_of(da) == 0700);

    char target[PATH_MAX];
    ssize_t n = readlink(dl, target, sizeof(target) - 1);
    assert(n > 0);
    target[n] = '\0';
    assert(strcmp(target, "top.txt") == 0);
    n = readlink(dd, target, sizeof(target) - 1);
    assert(n > 0);
    target[n] = '\0';
    assert(strcmp(target, "/nonexistent/target") == 0);

    free(nested);
    free(f1);
    free(f2);
    free(lnk);
    free(dangling);
    free(private_dir);
    free(d1);
    free(d2);
    free(dl);
    free(dd);
    free(da);
    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_dir_copies_tree_with_modes_and_links: PASS\n");
}

void
test_copy_dir_merges_into_existing_destination(void)
{
    char *dir = fs_mktmp("cpmerge");
    char *src = fs_join(dir, "src");
    char *dst = fs_join(dir, "dst");
    fs_mkdir_p(src);
    fs_mkdir_p(dst);

    char *s_new = fs_join(src, "new.txt");
    char *s_same = fs_join(src, "same.txt");
    char *d_same = fs_join(dst, "same.txt");
    char *d_keep = fs_join(dst, "keep.txt");
    fs_write(s_new, "new");
    fs_write(s_same, "from-src");
    fs_write(d_same, "old-dst");
    fs_write(d_keep, "keep");

    assert(copy_dir(src, dst));

    char *d_new = fs_join(dst, "new.txt");
    assert(content_is(d_new, "new"));
    assert(content_is(d_same, "from-src"));
    assert(content_is(d_keep, "keep"));

    free(d_new);
    free(s_new);
    free(s_same);
    free(d_same);
    free(d_keep);
    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_dir_merges_into_existing_destination: PASS\n");
}

void
test_copy_dir_replaces_mismatched_entry_types(void)
{
    char *dir = fs_mktmp("cptype");
    char *src = fs_join(dir, "src");
    char *dst = fs_join(dir, "dst");
    fs_mkdir_p(src);
    fs_mkdir_p(dst);

    char *s_dir = fs_join(src, "was_file");
    char *s_file = fs_join(src, "was_dir");
    char *s_link = fs_join(src, "was_dir_link");
    fs_mkdir_p(s_dir);
    char *s_inner = fs_join(s_dir, "inner");
    fs_write(s_inner, "inner");
    fs_write(s_file, "plain");
    assert(symlink("plain", s_link) == 0);

    char *d_file = fs_join(dst, "was_file");
    char *d_dir = fs_join(dst, "was_dir");
    char *d_dir_link = fs_join(dst, "was_dir_link");
    fs_write(d_file, "old file");
    fs_mkdir_p(d_dir);
    char *d_dir_inner = fs_join(d_dir, "old_inner");
    fs_write(d_dir_inner, "x");
    fs_mkdir_p(d_dir_link);

    assert(copy_dir(src, dst));

    struct stat st;
    assert(lstat(d_file, &st) == 0 && S_ISDIR(st.st_mode));
    assert(lstat(d_dir, &st) == 0 && S_ISREG(st.st_mode));
    assert(lstat(d_dir_link, &st) == 0 && S_ISLNK(st.st_mode));

    free(s_dir);
    free(s_file);
    free(s_link);
    free(s_inner);
    free(d_file);
    free(d_dir);
    free(d_dir_link);
    free(d_dir_inner);
    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_dir_replaces_mismatched_entry_types: PASS\n");
}

void
test_copy_dir_reports_missing_source(void)
{
    char *dir = fs_mktmp("cpmiss");
    char *src = fs_join(dir, "nope");
    char *dst = fs_join(dir, "dst");

    assert(!copy_dir(src, dst));
    assert(apg_last_error() && strstr(apg_last_error(), "nope"));

    struct stat st;
    assert(stat(dst, &st) != 0);

    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_dir_reports_missing_source: PASS\n");
}

void
test_copy_dir_stops_on_unreadable_file(void)
{
    if (geteuid() == 0)
    {
        printf("test_copy_dir_stops_on_unreadable_file: SKIP (root)\n");
        return;
    }

    char *dir = fs_mktmp("cpunread");
    char *src = fs_join(dir, "src");
    char *dst = fs_join(dir, "dst");
    fs_mkdir_p(src);
    char *secret = fs_join(src, "secret");
    fs_write(secret, "x");
    assert(chmod(secret, 0000) == 0);

    assert(!copy_dir(src, dst));
    assert(apg_last_error() && strstr(apg_last_error(), "secret"));

    free(secret);
    free(src);
    free(dst);
    fs_rmtree(dir);
    free(dir);
    printf("test_copy_dir_stops_on_unreadable_file: PASS\n");
}

void
test_concat_dirs_normalizes_separators(void)
{
    char *p = concat_dirs("a", "b");
    assert(p && strcmp(p, "a/b") == 0);
    free(p);

    p = concat_dirs("a/", "b");
    assert(p && strcmp(p, "a/b") == 0);
    free(p);

    p = concat_dirs("a//", "/b");
    assert(p && strcmp(p, "a/b") == 0);
    free(p);

    p = concat_dirs("/", "etc");
    assert(p && strcmp(p, "/etc") == 0);
    free(p);

    p = concat_dirs("/root", "");
    assert(p && strcmp(p, "/root/") == 0);
    free(p);

    p = concat("ab", "cd");
    assert(p && strcmp(p, "abcd") == 0);
    free(p);

    printf("test_concat_dirs_normalizes_separators: PASS\n");
}

void
test_collect_files_lists_relative_paths(void)
{
    char *dir = fs_mktmp("collect");
    char *nested = fs_join(dir, "usr/bin");
    fs_mkdir_p(nested);
    char *f1 = fs_join(dir, "top");
    char *f2 = fs_join(nested, "tool");
    char *lnk = fs_join(dir, "usr/link");
    fs_write(f1, "1");
    fs_write(f2, "2");
    assert(symlink("bin", lnk) == 0);

    int count = 0;
    char **files = collect_files(dir, &count);
    assert(files && count == 3);
    assert(fs_has_name(files, count, "/top"));
    assert(fs_has_name(files, count, "/usr/bin/tool"));
    assert(fs_has_name(files, count, "/usr/link"));
    fs_free_names(files, count);

    char *with_slash = fs_join(dir, "");
    files = collect_files(with_slash, &count);
    assert(count == 3);
    assert(fs_has_name(files, count, "/top"));
    assert(fs_has_name(files, count, "/usr/bin/tool"));
    fs_free_names(files, count);
    free(with_slash);

    files = collect_files("/nonexistent-apg-test-dir", &count);
    assert(count == 0);
    free(files);

    for (int i = 0; i < 40; i++)
    {
        char name[64];
        snprintf(name, sizeof(name), "bulk-%02d", i);
        char *p = fs_join(dir, name);
        fs_write(p, "x");
        free(p);
    }
    files = collect_files(dir, &count);
    assert(count == 43);
    fs_free_names(files, count);

    free(nested);
    free(f1);
    free(f2);
    free(lnk);
    fs_rmtree(dir);
    free(dir);
    printf("test_collect_files_lists_relative_paths: PASS\n");
}

void
test_remove_dir_recursive_does_not_follow_links(void)
{
    char *dir = fs_mktmp("rmrec");
    char *outside = fs_mktmp("rmout");
    char *outside_file = fs_join(outside, "survivor");
    fs_write(outside_file, "keep");

    char *tree = fs_join(dir, "tree/sub");
    fs_mkdir_p(tree);
    char *f = fs_join(tree, "file");
    fs_write(f, "x");
    char *lnk = fs_join(dir, "tree/outside_link");
    assert(symlink(outside, lnk) == 0);

    char *root = fs_join(dir, "tree");
    remove_dir_recursive(root);

    struct stat st;
    assert(lstat(root, &st) != 0);
    assert(stat(outside_file, &st) == 0);

    remove_dir_recursive(NULL);
    remove_dir_recursive("/nonexistent-apg-test-dir");

    free(tree);
    free(f);
    free(lnk);
    free(root);
    free(outside_file);
    fs_rmtree(dir);
    fs_rmtree(outside);
    free(dir);
    free(outside);
    printf("test_remove_dir_recursive_does_not_follow_links: PASS\n");
}
