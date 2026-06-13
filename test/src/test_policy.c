// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <apg/config.h>
#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

static struct db_handle *
open_tmp_db(char *buf)
{
    strcpy(buf, "/tmp/apg-test-XXXXXX");
    if (!mkdtemp(buf))
        return NULL;
    return db_open(buf);
}

static void
close_tmp_db(struct db_handle *db, const char *path)
{
    char f[PATH_MAX];
    db_close(db);
    snprintf(f, sizeof(f), "%s/data.mdb", path);
    unlink(f);
    snprintf(f, sizeof(f), "%s/lock.mdb", path);
    unlink(f);
    rmdir(path);
}

static struct package *
simple_pkg(const char *name)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta = package_metadata_new();
    assert(pkg->meta);
    pkg->meta->name = strdup(name);
    pkg->meta->version = strdup("1.0");
    pkg->pkg_path = strdup("/nonexistent/test.pkg");
    pkg->installed_by_hand = true;
    return pkg;
}

void
test_policy_unsigned(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = true,
                        .keyring_dir = "/nonexistent/keys"};
    trans_set_policy(trans, &p);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_UNSIGNED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_unsigned: PASS\n");
}

void
test_policy_no_sig_required(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = false};
    trans_set_policy(trans, &p);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_INSTALL_FAILED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_no_sig_required: PASS\n");
}

void
test_policy_clear(void)
{
    char db_path[PATH_MAX];
    struct db_handle *db = open_tmp_db(db_path);
    assert(db);

    struct package *pkg = simple_pkg("foo");
    struct apg_trans *trans = trans_new(db);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    install_policy p = {.require_signature = true,
                        .keyring_dir = "/nonexistent/keys"};
    trans_set_policy(trans, &p);
    trans_set_policy(trans, NULL);

    assert(trans_commit(trans, "/tmp") == TRANS_ERR_INSTALL_FAILED);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    printf("test_policy_clear: PASS\n");
}
