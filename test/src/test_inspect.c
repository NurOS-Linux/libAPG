// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/error.h>
#include <apg/package.h>
#include <apg/util.h>

#include "fs_helpers.h"

static char *
build_archive(const char *metadata)
{
    char *src = fs_mktmp("inspect-src");
    char *bin = fs_join(src, "data/usr/bin");
    char *share = fs_join(src, "data/usr/share/app/deep");
    char *empty = fs_join(src, "data/var/empty");
    char *scripts = fs_join(src, "scripts");
    fs_mkdir_p(bin);
    fs_mkdir_p(share);
    fs_mkdir_p(empty);
    fs_mkdir_p(scripts);

    char *meta = fs_join(src, "metadata.json");
    fs_write(meta, metadata);
    char *tool = fs_join(bin, "tool");
    fs_write(tool, "#!/bin/sh\n");
    char *hard = fs_join(bin, "tool-hard");
    assert(link(tool, hard) == 0);
    char *sym = fs_join(bin, "tool-link");
    assert(symlink("tool", sym) == 0);
    char *deep = fs_join(share, "file.txt");
    fs_write(deep, "deep");
    char *script = fs_join(scripts, "post-install");
    fs_write(script, "#!/bin/sh\n");

    char *archive = fs_join(src, "pkg.tar.gz");
    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd),
             "tar -czf '%s' -C '%s' metadata.json data scripts", archive, src);
    assert(system(cmd) == 0);

    free(bin);
    free(share);
    free(empty);
    free(scripts);
    free(meta);
    free(tool);
    free(hard);
    free(sym);
    free(deep);
    free(script);
    free(src);
    return archive;
}

static void
remove_archive(char *archive)
{
    char *slash = strrchr(archive, '/');
    *slash = '\0';
    fs_rmtree(archive);
    free(archive);
}

static const char *full_metadata =
    "{\"name\": \"inspect\", \"version\": \"3:1.2.3-4\", \"type\": \"app\", "
    "\"architecture\": \"x86_64\", \"description\": \"d\", "
    "\"dependencies\": [\"libx>=1.0\", \"liby\"], \"provides\": [\"virt\"], "
    "\"conf\": [\"etc/inspect.conf\"]}";

void
test_package_read_metadata_reads_archive(void)
{
    char *archive = build_archive(full_metadata);

    struct package_metadata *meta = package_read_metadata(archive);
    assert(meta);
    assert(strcmp(meta->name, "inspect") == 0);
    assert(strcmp(meta->version, "3:1.2.3-4") == 0);
    assert(strcmp(meta->architecture, "x86_64") == 0);
    assert(meta->dependencies.count == 2);
    assert(strcmp(meta->dependencies.items[0].name, "libx") == 0);
    assert(meta->provides.count == 1);
    assert(meta->conf.count == 1);
    package_metadata_free(meta);

    char *root = fs_mktmp("inspect-root");
    struct package *parsed = parse_package(archive, root);
    assert(parsed);
    meta = package_read_metadata(archive);
    assert(strcmp(meta->name, parsed->meta->name) == 0);
    assert(strcmp(meta->version, parsed->meta->version) == 0);
    assert(meta->dependencies.count == parsed->meta->dependencies.count);
    package_metadata_free(meta);
    package_free(parsed);
    fs_rmtree(root);
    free(root);

    remove_archive(archive);
    printf("test_package_read_metadata_reads_archive: PASS\n");
}

void
test_package_read_metadata_reports_failures(void)
{
    assert(package_read_metadata(NULL) == NULL);

    assert(package_read_metadata("/nonexistent/pkg.tar.gz") == NULL);
    assert(apg_last_error() && strstr(apg_last_error(), "archive"));

    char *dir = fs_mktmp("inspect-bad");
    char *garbage = fs_join(dir, "garbage");
    fs_write(garbage, "not an archive");
    assert(package_read_metadata(garbage) == NULL);

    char *bad_json = fs_join(dir, "bad-json.tar");
    FILE *f = fopen(bad_json, "wb");
    assert(f);
    fs_write_tar_entry(f, "metadata.json", "{\"name\": [1,");
    char end[1024] = {0};
    fwrite(end, 1, sizeof(end), f);
    fclose(f);
    assert(package_read_metadata(bad_json) == NULL);
    assert(apg_last_error() && strstr(apg_last_error(), "cannot be read"));

    char *no_meta = fs_join(dir, "no-meta.tar");
    f = fopen(no_meta, "wb");
    assert(f);
    fs_write_tar_entry(f, "data/file", "x");
    fwrite(end, 1, sizeof(end), f);
    fclose(f);
    assert(package_read_metadata(no_meta) == NULL);
    assert(apg_last_error() && strstr(apg_last_error(), "no metadata.json"));

    free(garbage);
    free(bad_json);
    free(no_meta);
    fs_rmtree(dir);
    free(dir);
    printf("test_package_read_metadata_reports_failures: PASS\n");
}

void
test_package_list_files_matches_install(void)
{
    char *archive = build_archive(full_metadata);

    char **files = NULL;
    int count = -1;
    assert(package_list_files(archive, &files, &count));
    assert(count == 4);
    assert(fs_has_name(files, count, "/usr/bin/tool"));
    assert(fs_has_name(files, count, "/usr/bin/tool-hard"));
    assert(fs_has_name(files, count, "/usr/bin/tool-link"));
    assert(fs_has_name(files, count, "/usr/share/app/deep/file.txt"));
    assert(!fs_has_name(files, count, "/var/empty"));
    assert(!fs_has_name(files, count, "/post-install"));

    char *root = fs_mktmp("inspect-install-root");
    struct package *pkg = parse_package(archive, root);
    assert(pkg);
    assert(install_package_in_root(pkg, root));
    assert(pkg->package_files.count == count);
    for (int i = 0; i < pkg->package_files.count; i++)
        assert(fs_has_name(files, count, pkg->package_files.items[i]));

    fs_free_names(files, count);
    package_free(pkg);
    fs_rmtree(root);
    free(root);
    remove_archive(archive);
    printf("test_package_list_files_matches_install: PASS\n");
}

void
test_package_list_files_edge_cases(void)
{
    char **files = (char **)0x1;
    int count = 7;
    assert(!package_list_files(NULL, &files, &count));
    assert(!package_list_files("/nonexistent/pkg.tar.gz", &files, &count));
    assert(files == NULL && count == 0);
    assert(!package_list_files("/nonexistent/pkg.tar.gz", NULL, &count));
    assert(!package_list_files("/nonexistent/pkg.tar.gz", &files, NULL));

    char *dir = fs_mktmp("inspect-edge");
    char end[1024] = {0};

    char *empty = fs_join(dir, "empty.tar");
    FILE *f = fopen(empty, "wb");
    assert(f);
    fs_write_tar_entry(f, "metadata.json", "{\"name\": \"e\"}");
    fwrite(end, 1, sizeof(end), f);
    fclose(f);
    assert(package_list_files(empty, &files, &count));
    assert(files == NULL && count == 0);

    char *prefixed = fs_join(dir, "prefixed.tar");
    f = fopen(prefixed, "wb");
    assert(f);
    fs_write_tar_entry(f, "./data/usr/bin/x", "x");
    fs_write_tar_entry(f, "data/", "");
    fwrite(end, 1, sizeof(end), f);
    fclose(f);
    assert(package_list_files(prefixed, &files, &count));
    assert(count == 1 && strcmp(files[0], "/usr/bin/x") == 0);
    fs_free_names(files, count);

    char *evil = fs_join(dir, "evil.tar");
    f = fopen(evil, "wb");
    assert(f);
    fs_write_tar_entry(f, "data/usr/bin/ok", "x");
    fs_write_tar_entry(f, "data/../escape", "x");
    fwrite(end, 1, sizeof(end), f);
    fclose(f);
    assert(!package_list_files(evil, &files, &count));
    assert(apg_last_error() && strstr(apg_last_error(), "unsafe"));
    assert(files == NULL && count == 0);

    free(empty);
    free(prefixed);
    free(evil);
    fs_rmtree(dir);
    free(dir);
    printf("test_package_list_files_edge_cases: PASS\n");
}
