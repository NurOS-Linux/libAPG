// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/error.h>
#include <apg/package.h>

#include "fs_helpers.h"

struct fixture
{
    char db_path[PATH_MAX];
    struct db_handle *db;
    char *src;
    char *archive;
    char *root;
};

static char *
build_archive(const char *name, const char *tool_body)
{
    char *src = fs_mktmp("content-src");
    char *bin = fs_join(src, "data/usr/bin");
    char *etc = fs_join(src, "data/etc");
    fs_mkdir_p(bin);
    fs_mkdir_p(etc);

    char *meta = fs_join(src, "metadata.json");
    char json[512];
    snprintf(json, sizeof(json),
             "{\"name\": \"%s\", \"version\": \"1.0.0\", \"type\": \"app\", "
             "\"conf\": [\"etc/app.conf\"]}",
             name);
    fs_write(meta, json);

    char *tool = fs_join(bin, "tool");
    fs_write(tool, tool_body);
    assert(chmod(tool, 0755) == 0);
    char *data = fs_join(bin, "data.txt");
    fs_write(data, "0123456789");
    char *link = fs_join(bin, "link");
    assert(symlink("tool", link) == 0);
    char *conf = fs_join(etc, "app.conf");
    fs_write(conf, "default=1\n");

    char *archive = fs_join(src, "pkg.tar.gz");
    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), "tar -czf '%s' -C '%s' metadata.json data",
             archive, src);
    assert(system(cmd) == 0);

    free(bin);
    free(etc);
    free(meta);
    free(tool);
    free(data);
    free(link);
    free(conf);
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

static void
setup(struct fixture *fx)
{
    fx->db = fs_open_db(fx->db_path);
    assert(fx->db);
    fx->root = fs_mktmp("content-root");
    fx->archive = build_archive("content-pkg", "#!/bin/sh\necho tool\n");

    struct package *pkg = parse_package(fx->archive, fx->root);
    assert(pkg);
    assert(install_package_in_root(pkg, fx->root));
    assert(db_add(fx->db, pkg));
    package_free(pkg);
}

static void
teardown(struct fixture *fx)
{
    remove_archive(fx->archive);
    fs_rmtree(fx->root);
    free(fx->root);
    fs_close_db(fx->db, fx->db_path);
}

static bool
has_issue(const struct db_content_issue *issues, int count,
          db_content_kind_t kind, const char *path)
{
    for (int i = 0; i < count; i++)
    {
        const struct db_content_issue *issue =
            db_content_issue_at(issues, count, i);
        if (db_content_issue_kind(issue) == kind &&
            strcmp(db_content_issue_path(issue), path) == 0)
            return true;
    }
    return false;
}

void
test_verify_against_archive_accepts_clean_install(void)
{
    struct fixture fx;
    setup(&fx);

    struct db_content_issue *issues = NULL;
    int count = -1;
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(issues == NULL && count == 0);

    char *conf = fs_join(fx.root, "etc/app.conf");
    fs_write(conf, "default=2\nlocal=1\n");
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(issues == NULL && count == 0);
    free(conf);

    db_content_free(NULL, 0);
    assert(db_content_issue_at(NULL, 0, 0) == NULL);

    teardown(&fx);
    printf("test_verify_against_archive_accepts_clean_install: PASS\n");
}

void
test_verify_against_archive_reports_differences(void)
{
    struct fixture fx;
    setup(&fx);

    char *tool = fs_join(fx.root, "usr/bin/tool");
    char *data = fs_join(fx.root, "usr/bin/data.txt");
    char *link = fs_join(fx.root, "usr/bin/link");

    fs_write(tool, "#!/bin/sh\necho evil\n");
    assert(chmod(tool, 0755) == 0);
    fs_write(data, "01234567XX");
    assert(unlink(link) == 0);
    assert(symlink("elsewhere", link) == 0);

    struct db_content_issue *issues = NULL;
    int count = 0;
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(count == 3);
    assert(has_issue(issues, count, DB_CONTENT_MODIFIED, "/usr/bin/tool"));
    assert(has_issue(issues, count, DB_CONTENT_MODIFIED, "/usr/bin/data.txt"));
    assert(has_issue(issues, count, DB_CONTENT_MODIFIED, "/usr/bin/link"));
    db_content_free(issues, count);

    fs_write(tool, "#!/bin/sh\necho tool\n");
    assert(chmod(tool, 0755) == 0);
    fs_write(data, "short");
    assert(chmod(data, 0600) == 0);
    assert(unlink(link) == 0);
    assert(symlink("tool", link) == 0);
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(count == 1);
    assert(has_issue(issues, count, DB_CONTENT_MODIFIED, "/usr/bin/data.txt"));
    db_content_free(issues, count);

    fs_write(data, "0123456789");
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(count == 1);
    assert(
        has_issue(issues, count, DB_CONTENT_MODE_CHANGED, "/usr/bin/data.txt"));
    db_content_free(issues, count);

    assert(chmod(data, 0644) == 0);
    assert(unlink(tool) == 0);
    assert(unlink(link) == 0);
    assert(mkdir(link, 0755) == 0);
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(count == 2);
    assert(has_issue(issues, count, DB_CONTENT_MISSING, "/usr/bin/tool"));
    assert(has_issue(issues, count, DB_CONTENT_TYPE_CHANGED, "/usr/bin/link"));
    db_content_free(issues, count);

    assert(rmdir(link) == 0);
    assert(symlink("tool", link) == 0);
    assert(unlink(data) == 0);
    assert(mkdir(data, 0755) == 0);
    fs_write(tool, "#!/bin/sh\necho tool\n");
    assert(chmod(tool, 0755) == 0);
    assert(db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                     &issues, &count));
    assert(count == 1);
    assert(
        has_issue(issues, count, DB_CONTENT_TYPE_CHANGED, "/usr/bin/data.txt"));
    db_content_free(issues, count);

    free(tool);
    free(data);
    free(link);
    teardown(&fx);
    printf("test_verify_against_archive_reports_differences: PASS\n");
}

void
test_verify_against_archive_rejects_bad_requests(void)
{
    struct fixture fx;
    setup(&fx);

    struct db_content_issue *issues = NULL;
    int count = 0;

    assert(!db_verify_against_archive(fx.db, "not-installed", fx.archive,
                                      fx.root, &issues, &count));
    assert(apg_last_error() && strstr(apg_last_error(), "not-installed"));

    char *other = build_archive("another-pkg", "#!/bin/sh\n");
    assert(!db_verify_against_archive(fx.db, "content-pkg", other, fx.root,
                                      &issues, &count));
    assert(apg_last_error() && strstr(apg_last_error(), "another-pkg"));
    assert(issues == NULL && count == 0);
    remove_archive(other);

    assert(!db_verify_against_archive(fx.db, "content-pkg",
                                      "/nonexistent/pkg.tar.gz", fx.root,
                                      &issues, &count));
    assert(apg_last_error() && strstr(apg_last_error(), "archive"));

    char *garbage = fs_join(fx.root, "garbage.tar.gz");
    fs_write(garbage, "this is not an archive");
    assert(!db_verify_against_archive(fx.db, "content-pkg", garbage, fx.root,
                                      &issues, &count));
    free(garbage);

    char *evil = fs_join(fx.root, "evil.tar");
    FILE *tar = fopen(evil, "wb");
    assert(tar);
    fs_write_tar_entry(tar, "metadata.json",
                       "{\"name\": \"content-pkg\", \"version\": \"1\"}");
    fs_write_tar_entry(tar, "data/../escape", "x");
    char end[1024] = {0};
    fwrite(end, 1, sizeof(end), tar);
    fclose(tar);
    assert(!db_verify_against_archive(fx.db, "content-pkg", evil, fx.root,
                                      &issues, &count));
    assert(apg_last_error() && strstr(apg_last_error(), "unsafe"));
    assert(issues == NULL && count == 0);
    free(evil);

    assert(!db_verify_against_archive(NULL, "content-pkg", fx.archive, fx.root,
                                      &issues, &count));
    assert(!db_verify_against_archive(fx.db, NULL, fx.archive, fx.root, &issues,
                                      &count));
    assert(!db_verify_against_archive(fx.db, "content-pkg", NULL, fx.root,
                                      &issues, &count));
    assert(!db_verify_against_archive(fx.db, "content-pkg", fx.archive, NULL,
                                      &issues, &count));
    assert(!db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                      NULL, &count));
    assert(!db_verify_against_archive(fx.db, "content-pkg", fx.archive, fx.root,
                                      &issues, NULL));

    teardown(&fx);
    printf("test_verify_against_archive_rejects_bad_requests: PASS\n");
}
