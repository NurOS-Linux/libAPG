// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "db_priv.h"
#include "../../include/apg/db.h"
#include "../../include/apg/package.h"
#include "../../include/util.h"

struct db_verify_issue *
db_verify(struct db_handle *db, const char *root_path, int *count)
{
    *count = 0;
    if (!db || !root_path)
        return NULL;

    int pkg_count = 0;
    struct package **pkgs = db_list(db, &pkg_count);
    if (!pkgs)
        return NULL;

    struct db_verify_issue *issues = NULL;
    int issue_count = 0;
    int issue_cap = 0;

    for (int i = 0; i < pkg_count; i++)
    {
        struct package *pkg = pkgs[i];
        if (!pkg->meta || !pkg->meta->name || pkg->package_files.count == 0)
            continue;

        char **missing = NULL;
        int missing_count = 0;
        int missing_cap = 0;

        for (int j = 0; j < pkg->package_files.count; j++)
        {
            const char *rel = pkg->package_files.items[j];
            if (!rel)
                continue;

            char *full = concat_dirs(root_path, rel);
            if (!full)
                goto oom;

            struct stat st;
            bool exists = stat(full, &st) == 0;
            free(full);

            if (!exists)
            {
                if (missing_count == missing_cap)
                {
                    int new_cap = missing_cap == 0 ? 4 : missing_cap * 2;
                    char **tmp = realloc(missing, new_cap * sizeof(*tmp));
                    if (!tmp)
                        goto oom;
                    missing = tmp;
                    missing_cap = new_cap;
                }
                missing[missing_count] = strdup(rel);
                if (!missing[missing_count])
                    goto oom;
                missing_count++;
            }
        }

        if (missing_count == 0)
        {
            free(missing);
            continue;
        }

        if (issue_count == issue_cap)
        {
            int new_cap = issue_cap == 0 ? 4 : issue_cap * 2;
            struct db_verify_issue *tmp =
                realloc(issues, new_cap * sizeof(*tmp));
            if (!tmp)
                goto oom;
            issues = tmp;
            issue_cap = new_cap;
        }

        struct db_verify_issue *issue = &issues[issue_count++];
        issue->pkg_name = strdup(pkg->meta->name);
        if (!issue->pkg_name)
            goto oom;
        issue->missing_files = missing;
        issue->missing_count = missing_count;
    }

    for (int i = 0; i < pkg_count; i++)
        package_free(pkgs[i]);
    free(pkgs);

    *count = issue_count;
    return issues;

oom:
    db_verify_free(issues, issue_count);
    for (int i = 0; i < pkg_count; i++)
        package_free(pkgs[i]);
    free(pkgs);
    return NULL;
}

void
db_verify_free(struct db_verify_issue *issues, int count)
{
    if (!issues)
        return;
    for (int i = 0; i < count; i++)
    {
        free(issues[i].pkg_name);
        for (int j = 0; j < issues[i].missing_count; j++)
            free(issues[i].missing_files[j]);
        free(issues[i].missing_files);
    }
    free(issues);
}
